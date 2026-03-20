#include "assets.h"

#include "platform/platform.h"

#include "texture_serializer.h"
#include "shader_serializer.h"
#include "model_serializer.h"
#include "sound_serializer.h"

using namespace ast;

String AssetLoadContext::system_file_path() const
{
	return assets.get_system_file_path(metadata.file_path);
}

AssetManager::AssetManager()
	: device(nullptr)
	, serializers{}
	, path_to_handle()
	, async_upload_counter(nullptr)
	, upload_queue()
	, upload_mutex()
	, dependency_queue()
	, dependency_mutex()
	, loading_cv()
	, loading_mutex()
	, mount_points()
	, asset_arena()
	, allocation_mutex()
	, asset_records()
	, free_asset_indices()
{
	serializers[ASSET_TYPE_TEXTURE] = get_texture_serializer();
	serializers[ASSET_TYPE_SHADER] = get_shader_serializer();
	serializers[ASSET_TYPE_MODEL] = get_model_serializer();
	serializers[ASSET_TYPE_SOUND] = get_sound_serializer();
}

AssetManager::~AssetManager()
{
}

void AssetManager::init(ArenaView &&arena, gfx::Device *device)
{
	this->device = device;
	this->asset_arena = std::move(arena);

	async_upload_counter = job::alloc_counter();
}

void AssetManager::destroy()
{
	for (AssetRecord &record : asset_records)
		record.asset->unload();

	asset_arena.destroy();
	job::free_counter(async_upload_counter);
}

void AssetManager::destroy_asset(const AssetHandle &handle)
{
	assert(is_valid(handle));

	asset_records[handle.index].asset->unload();
	asset_records[handle.index].asset = nullptr;

	free_asset_indices.push(handle.index);
}

AssetHandle AssetManager::from_file_path(const String &path)
{
	std::lock_guard<std::mutex> lock(allocation_mutex);

	if (path_to_handle.find(path) != path_to_handle.end())
		return path_to_handle[path];

	AssetHandle handle = allocate_asset(path);

	path_to_handle[path] = handle;

	return handle;
}

bool AssetManager::is_loaded(const AssetHandle &handle) const
{
	assert(is_valid(handle));
	return asset_state_is_loaded(asset_records[handle.index].state); // TODO: also check if asset != placeholder.
}

bool AssetManager::is_loading(const AssetHandle &handle)
{
	assert(is_valid(handle));
	return asset_state_is_loading(asset_records[handle.index].state);
}

void AssetManager::load_now(const AssetHandle &handle, AssetType type)
{
	assert(is_valid(handle));

	if (!asset_state_needs_load(asset_records[handle.index].state))
		return;

	job::JobCounter *counter = job::alloc_counter();

	load_asset_internal(handle, type, counter);

	job::yield_on_counter(counter);

	while (true) {
		if (asset_records[handle.index].state == ASSET_STATE_READY)
			break;
		
		resolve_pending_dependencies(counter);

		job::yield_on_counter(counter);

		flush_uploads();
	}

	job::free_counter(counter);
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

	VirtualArena *virtual_arena = new VirtualArena();
	virtual_arena->reserve(MEGABYTES(64));

	ArenaView arena = virtual_arena->arena(MEGABYTES(64));

	AssetLoadContext context = {
		.assets = *load_param->assets,
		.metadata = load_param->metadata,
		.arena = arena
	};

	IAssetSerializer *serializer = load_param->assets->get_serializer(load_param->type);
	
	AssetLoadResult result = serializer->load(context);
	
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
	load_param->assets->push_for_dependency_resolution(std::move(upload));
	
	delete load_param;
}

void AssetManager::load_async(const AssetHandle &handle, AssetType type)
{
	assert(is_valid(handle));

	AssetRecord &record = asset_records[handle.index];

	if (asset_state_needs_load(record.state))
		load_asset_internal(handle, type, async_upload_counter);
}

void AssetManager::reload_async(const AssetHandle &handle, AssetType type)
{
	assert(is_valid(handle));

	AssetRecord &record = asset_records[handle.index];

	if (!asset_state_is_loading(record.state))
		load_asset_internal(handle, type, async_upload_counter);
}

void AssetManager::load_asset_internal(const AssetHandle &handle, AssetType type, job::JobCounter *counter)
{
	asset_records[handle.index].state = ASSET_STATE_LOADING_DATA;

	AssetMetaData metadata = {};
	metadata.file_path = get_path(handle);

	AssetLoadJobParam *param = new AssetLoadJobParam();
	param->assets = this;
	param->metadata = metadata;
	param->handle = handle;
	param->type = type;

	job::JobDecl decl(asset_load_job, param);
	job::kick_job(decl, &counter);
}

void AssetManager::poll_hot_reloads()
{
	for (int i = 0; i < asset_records.size(); i++) {
		AssetRecord &record = asset_records[i];

		if (!record.asset)
			continue;

		String path = get_system_file_path(record.path);
		u64 current_time = platform::file_last_write_time(path.c_str());

		for (auto &path : record.watch_paths) {
			u64 t = platform::file_last_write_time(path.c_str());
			if (t > current_time)
				current_time = t;
		}

		if (current_time > record.last_write_time && current_time != 0) {
			AssetHandle handle = record.asset->get_handle();
			AssetType type = record.asset->get_asset_type();

			record.last_write_time = current_time;

			reload_async(handle, type);
		}
	}

	resolve_pending_dependencies(async_upload_counter);
}

void AssetManager::wait_for_async_uploads()
{
	job::yield_on_counter(async_upload_counter);
}

void AssetManager::push_upload(AssetUpload &&upload)
{
	std::lock_guard<std::mutex> lock(upload_mutex);
	upload_queue.push_back(std::move(upload));
}

void AssetManager::push_for_dependency_resolution(AssetUpload &&upload)
{
	std::lock_guard<std::mutex> lock(dependency_mutex);
	dependency_queue.push_back(std::move(upload));
}

void AssetManager::notify_dependents(const AssetHandle &handle, bool failed)
{
	std::lock_guard<std::mutex> lock(dependency_mutex);
	notify_dependents_no_lock(handle, failed);
}

void AssetManager::notify_dependents_no_lock(const AssetHandle &handle, bool failed)
{
	assert(is_valid(handle));

	AssetRecord &record = asset_records[handle.index];

	for (auto &parent_handle : record.dependents) {
		AssetRecord &parent = asset_records[parent_handle.index];

		if (failed) {
			parent.state = ASSET_STATE_FAILED;
			notify_dependents_no_lock(parent_handle, true);
		} else {
			if (parent.pending_dependencies > 0)
				parent.pending_dependencies--;

			if (parent.pending_dependencies == 0 && parent.state == ASSET_STATE_WAITING_FOR_DEPENDENCIES) {
				parent.state = ASSET_STATE_WAITING_FOR_GPU;
				push_upload(std::move(parent.stashed_upload_data));
			}
		}
	}

	record.dependents.clear();
}

void AssetManager::resolve_pending_dependencies(job::JobCounter *counter)
{
	std::lock_guard<std::mutex> lock(dependency_mutex);

	for (AssetUpload &upload : dependency_queue) {
		AssetRecord &record = asset_records[upload.handle.index];
	
		if (upload.result.failed) {
			record.state = ASSET_STATE_FAILED;
			notify_dependents_no_lock(upload.handle, true);
			continue;
		}

		u32 unresolved_count = 0;

		for (auto &dep_handle : upload.result.dependencies) {
			AssetRecord &dep_record = asset_records[dep_handle.index];
			
			if (!asset_state_is_loading(dep_record.state) && asset_state_needs_load(dep_record.state))
				load_asset_internal(dep_handle, ASSET_TYPE_TEXTURE, counter);

			if (!asset_state_is_finalized(dep_record.state)) {
				unresolved_count++;
				dep_record.dependents.push_back(upload.handle);
			}
		}

		if (unresolved_count > 0) {
			record.state = ASSET_STATE_WAITING_FOR_DEPENDENCIES;
			record.pending_dependencies = unresolved_count;
			record.stashed_upload_data = std::move(upload);
		} else {
			record.state = ASSET_STATE_WAITING_FOR_GPU;
			push_upload(std::move(upload));
		}
	}

	dependency_queue.clear();
}

void AssetManager::flush_uploads()
{
	if (!upload_mutex.try_lock())
		return;

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

		if (batch_stage_size > 0)
			staging_buffer = device->alloc_stage(batch_stage_size);

		device->submit_graphics_immediate([&](gfx::CommandBuffer &cmd) {
			for (int i = 0; i < batch_count; i++) {
				AssetUpload &upload = uploads_pending[base_index + i];

				IAssetSerializer *serializer = serializers[upload.type];
				
				AssetRecord &record = asset_records[upload.handle.index];
				Asset *asset = record.asset;
				
				bool need_new_asset = asset == nullptr;

				AssetLoadContext context = {
					.assets = *this,
					.metadata = upload.metadata,
					.arena = asset_arena
				};

				if (upload.result.failed) {
					asset = get_fallback_asset(upload.type);
				} else {
					if (need_new_asset) {
						debug_log("Creating %s Asset: %s...",
							get_string_from_asset_type(upload.type).c_str(),
							upload.metadata.file_path.c_str()
						);
					} else {
						debug_log("Reloading %s Asset: %s...",
							get_string_from_asset_type(upload.type).c_str(),
							upload.metadata.file_path.c_str()
						);
					}

					asset = serializer->finalize(context, upload.result, asset, *device);

					if (need_new_asset) {
						asset->handle = upload.handle;
						record.asset = asset;
					}

					serializer->gpu_upload(asset, context, upload.result, cmd, staging_buffer, stage_base);
					serializer->dispose(upload.result);

					record.watch_paths = upload.result.watch_paths;

					record.last_write_time = platform::file_last_write_time(get_system_file_path(record.path).c_str());

					for (auto &path : record.watch_paths) {
						u64 t = platform::file_last_write_time(path.c_str());
						if (t > record.last_write_time)
							record.last_write_time = t;
					}

					record.state = ASSET_STATE_READY;

					notify_dependents(upload.handle, false);

					stage_base += memory_align_up(upload.result.stage_size, 16);
				}

				upload.arena.destroy();

				upload.scratch->free();
				delete upload.scratch;

				loading_cv.notify_all();
			}
		});

		base_index += batch_count;

		if (staging_buffer)
			device->destroy_buffer(staging_buffer);
	}
}

bool AssetManager::is_valid(const AssetHandle &handle) const
{
	return
		(handle.index < asset_records.size()) &&
		(asset_records[handle.index].generation == (handle.generation + 1));
}

bool AssetManager::is_placeholder(const AssetHandle &handle) const
{
	return false; // TODO
}

String AssetManager::get_path(const AssetHandle &handle) const
{
	assert(is_valid(handle));
	return asset_records[handle.index].path;
}

AssetHandle AssetManager::allocate_asset(const String &path)
{
	u32 index;

	if (!free_asset_indices.empty()) {
		index = free_asset_indices.top();
		free_asset_indices.pop();
	} else {
		index = asset_records.size();
		asset_records.emplace_back();
	}

	AssetHandle handle = {};
	handle.index = index;
	handle.generation = asset_records[index].generation;

	asset_records[index].asset = nullptr;
	asset_records[index].path = path;
	asset_records[index].generation++;
	asset_records[index].last_write_time = 0;

	return handle;
}

void AssetManager::create_fallbacks()
{
	// TODO
}

Asset *AssetManager::get_fallback_asset(AssetType type)
{
	return nullptr; // TODO
}

IAssetSerializer *AssetManager::get_serializer(AssetType type) const
{
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
