#include "assets.h"

#include "platform/platform.h"

#include "texture_serializer.h"
#include "shader_serializer.h"
#include "model_serializer.h"

using namespace ast;

String AssetLoadContext::system_file_path() const
{
	return assets.get_system_file_path(metadata.file_path);
}

AssetManager::AssetManager()
	: device(nullptr)
	, assets()
	, serializers{}
	, path_to_handle()
	, upload_queue()
	, upload_counter(nullptr)
	, upload_mutex()
	, loading_assets()
	, loading_cv()
	, loading_mutex()
	, mount_points()
	, asset_arena()
{
	serializers[ASSET_TYPE_TEXTURE] = get_texture_serializer();
	serializers[ASSET_TYPE_SHADER] = get_shader_serializer();
	serializers[ASSET_TYPE_MODEL] = get_model_serializer();
}

AssetManager::~AssetManager()
{
}

void AssetManager::init(ArenaView &&arena, gfx::Device *device)
{
	this->device = device;
	
	this->asset_arena = std::move(arena);

	upload_counter = job::alloc_counter();
}

void AssetManager::destroy()
{
	asset_arena.destroy();
	job::free_counter(upload_counter);
}

void AssetManager::destroy_asset(const AssetHandle &handle)
{
	assets.remove(handle);
}

AssetHandle AssetManager::from_file_path(const String &path)
{
	if (path_to_handle.find(path) != path_to_handle.end())
		return path_to_handle[path];

	std::lock_guard<std::mutex> lock(loading_mutex);

	AssetHandle handle = assets.add(nullptr, path);

	loading_assets[handle.index] = false;
	path_to_handle[path] = handle;

	return handle;
}

bool AssetManager::is_loaded(const AssetHandle &handle) const
{
	if (!assets.is_valid(handle))
		return false;

	Asset *a = assets.get(handle);

	return a != nullptr; // TODO: also check if a != placeholder when adding those.
}

bool AssetManager::is_loading(const AssetHandle &handle)
{
	std::lock_guard<std::mutex> lock(loading_mutex);
	return loading_assets.at(handle.index);
}

void AssetManager::load_now(const AssetHandle &handle, AssetType type)
{
	if (is_loaded(handle) || is_loading(handle))
		return;

	{
		std::unique_lock<std::mutex> lock(loading_mutex);

		if (loading_assets.at(handle.index)) {
			loading_cv.wait(lock, [&] {
				return !loading_assets.at(handle.index);
			});
			return;
		}

		loading_assets[handle.index] = true;
	}

	AssetMetaData metadata = {};
	metadata.file_path = assets.get_path(handle);

	VirtualArena *virtual_arena = new VirtualArena();
	virtual_arena->reserve(MEGABYTES(64));

	ArenaView arena = virtual_arena->view(MEGABYTES(64));

	AssetLoadContext context = {
		.assets = *this,
		.metadata = metadata,
		.arena = arena
	};
	
	AssetLoadResult result = serializers[type].load(context);

	if (result.failed)
		debug_log("Failed to load %s asset: %s", get_string_from_asset_type(type).c_str(), metadata.file_path.c_str());

	AssetUpload upload = {};
	upload.scratch = virtual_arena;
	upload.arena = std::move(arena);
	upload.metadata = metadata;
	upload.handle = handle;
	upload.type = type;
	upload.result = result;

	push_upload(std::move(upload));

	flush_uploads();
}

struct AssetLoadJobParam {
	AssetManager *assets;
	AssetMetaData metadata;
	AssetHandle handle;
	AssetType type;
};

static JOB_ENTRY_POINT(asset_load_job)
{
	AssetLoadJobParam *load_param = (AssetLoadJobParam *)param;

	/*
	while (load_param->assets->memory_pressure > AssetManager::MAX_MEMORY_PRESSURE) {
		if (job::is_main_thread())
			load_param->assets->flush_uploads();
		else
			platform::yield_thread();
	}
	*/

	VirtualArena *virtual_arena = new VirtualArena();
	virtual_arena->reserve(MEGABYTES(64));

	ArenaView arena = virtual_arena->view(MEGABYTES(64));

	AssetLoadContext context = {
		.assets = *load_param->assets,
		.metadata = load_param->metadata,
		.arena = arena
	};

	const AssetSerializer &serializer = load_param->assets->get_serializer(load_param->type);

	AssetLoadResult result = serializer.load(context);

	if (result.failed)
		debug_log("Failed to load %s asset: %s", get_string_from_asset_type(load_param->type).c_str(), load_param->metadata.file_path.c_str());
	
	AssetUpload upload = {};
	upload.scratch = virtual_arena;
	upload.arena = std::move(arena);
	upload.metadata = load_param->metadata;
	upload.handle = load_param->handle;
	upload.type = load_param->type;
	upload.result = result;

	// Have to push regardless of failure because of promise that
	// will get loaded one way or another.
	// If we didn't do this, assets that fail to load will have
	// their associated handles in a permanent loading state.
	load_param->assets->push_upload(std::move(upload));
	
	delete load_param;
}

void AssetManager::load_async(const AssetHandle &handle, AssetType type)
{
	if (!is_loaded(handle) && !is_loading(handle))
		load_asset_internal(handle, type);
}

void AssetManager::reload_async(const AssetHandle &handle, AssetType type)
{
	if (!is_loading(handle))
		load_asset_internal(handle, type);
}

void AssetManager::load_asset_internal(const AssetHandle &handle, AssetType type)
{
	{
		std::unique_lock<std::mutex> lock(loading_mutex);
		loading_assets[handle.index] = true;
	}

	AssetMetaData metadata = {};
	metadata.file_path = assets.get_path(handle);

	AssetLoadJobParam *param = new AssetLoadJobParam();
	param->assets = this;
	param->metadata = metadata;
	param->handle = handle;
	param->type = type;

	job::JobDecl decl(asset_load_job, param);
	job::kick_job(decl, &upload_counter);
}

void AssetManager::poll_hot_reloads()
{
	for (int i = 0; i < assets.list.size(); i++) {
		auto &record = assets.list[i];

		if (!record.asset)
			continue;

		String path = get_system_file_path(record.path);
		u64 current_time = platform::file_last_write_time(path.c_str());

		if (current_time > record.last_write_time && current_time != 0) {
			AssetHandle handle = record.asset->get_handle();
			AssetType type = record.asset->get_asset_type();

			record.last_write_time = current_time;

			reload_async(handle, type);
		}
	}
}

void AssetManager::wait_for_async_uploads()
{
	job::yield_on_counter(upload_counter);
}

void AssetManager::push_upload(AssetUpload &&upload)
{
	std::lock_guard<std::mutex> lock(upload_mutex);
	upload_queue.push_back(std::move(upload));

//	memory_pressure += upload.result.stage_size;
}

void AssetManager::flush_uploads()
{
	upload_mutex.lock();

	if (upload_queue.empty()) {
		upload_mutex.unlock();
		return;
	}

	// Copy over uploads to unlock ASAP.
	Vector<AssetUpload> uploads_pending = std::move(upload_queue);
	upload_queue.clear();
	upload_mutex.unlock();

	assert(device);

	const u32 uploads_pending_count = uploads_pending.size();
	
	u32 base_index = 0;

	while (base_index < uploads_pending_count) {
		u64 batch_stage_size = 0;
		u32 batch_count = 0;

		for (u32 i = base_index; i < uploads_pending_count; i++) {
			const u64 upload_size = uploads_pending[i].result.stage_size;
			const u64 aligned_size = memory_align_up(upload_size, 16);

			// TODO: We make an exception for huge assets (larger than 1 chunk)
			//       And just process them all at once.
			if (aligned_size > GPU_UPLOAD_CHUNK_SIZE && batch_stage_size == 0) {
				batch_stage_size = aligned_size;
				batch_count = 1;
				break;
			}

			if (batch_stage_size + aligned_size > GPU_UPLOAD_CHUNK_SIZE)
				break; // Overflow.

			batch_stage_size += aligned_size;
			batch_count++;
		}
		
		u64 stage_base = 0;
	
		gfx::GpuBuffer *staging_buffer = nullptr;

		if (batch_stage_size)
			staging_buffer = device->alloc_stage(batch_stage_size);

		device->submit_graphics_immediate([&](gfx::CommandBuffer &cmd) {
			for (int i = 0; i < batch_count; i++) {
				auto &req = uploads_pending[base_index + i];

				const AssetSerializer &serializer = serializers[req.type];

				Asset *asset = assets.get(req.handle);
				
				AssetLoadContext context = {
					.assets = *this,
					.metadata = req.metadata,
					.arena = asset_arena
				};

				if (req.result.failed) {
					asset = get_fallback_asset(req.type);
				} else {
					if (!asset) {
						debug_log("Creating %s Asset: %s...",
							get_string_from_asset_type(req.type).c_str(),
							req.metadata.file_path.c_str()
						);

						asset = serializer.allocate(context, req.result, *device);
						asset->handle = req.handle;

						assets.set(req.handle, asset);
					} else {
						debug_log("Reloading %s Asset: %s...",
							get_string_from_asset_type(req.type).c_str(),
							req.metadata.file_path.c_str()
						);
					}

					serializer.upload(
						asset,
						context, req.result,
						cmd,
						staging_buffer, stage_base
					);

					if (asset->has_flag(ASSET_FLAG_INVALID))
						debug_log("Invalid Asset!");

					stage_base += memory_align_up(req.result.stage_size, 16);
				}

				/*
				if (req.result.data)
					serializer.clean_up(req.result.data);
				*/
				
				req.arena.destroy();

				req.scratch->free();
				delete req.scratch;

				{
					std::unique_lock<std::mutex> lock(loading_mutex);
					loading_assets[req.handle.index] = false;
					loading_cv.notify_all();
				}

//				memory_pressure -= req.result.stage_size;
			}
		});

		base_index += batch_count;

		if (staging_buffer)
			device->destroy_buffer(staging_buffer);
	}
}

bool AssetManager::is_valid(const AssetHandle &handle)
{
	return assets.is_valid(handle);
}

bool AssetManager::is_placeholder(const AssetHandle &handle) const
{
	return false; // TODO
}

void AssetManager::create_fallbacks()
{
	// TODO
}

Asset *AssetManager::get_fallback_asset(AssetType type)
{
	return nullptr; // TODO
}

const AssetSerializer &AssetManager::get_serializer(AssetType type) const
{
	assert(0 <= type && type < ASSET_TYPE_MAX_ENUM);
	return serializers[type];
}

void AssetManager::mount(const String &prefix, const String &physical_directory)
{
	String dir = physical_directory;

	if (dir.back() != '/' && dir.back() != '\\')
		dir += "/";

	mount_points[prefix] = dir;
}

String AssetManager::get_system_file_path(const String &path) const
{
	u64 separator_index = path.find("://");

	if (separator_index != String::npos) {
		String prefix = path.substr(0, separator_index); // assets, engine, ...
		String relative_path = path.substr(separator_index + 3);

		auto it = mount_points.find(prefix);

		if (it != mount_points.end())
			return it->second + relative_path;
		else
			debug_log("Unrecognised asset prefix in path: \"%s\"", path.c_str());
	}

	// Fallback...
	return path;
}
