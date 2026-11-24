#include "asset/asset_system.h"
#include "graphics/device.h"
#include "os/file_system.h"
#include "io/path.h"
#include "core/str.h"

static ast::AssetSystem *selected_system = nullptr;

ast::AssetSystem::AssetSystem(LogChannel log_channel_)
	: log_channel(log_channel_)
	, records(new Record[ASSETS_MAX_RECORDS])
	, record_pool(ASSETS_MAX_RECORDS)
{
	loaders[AssetType_Texture].loader = &texture_loader;
	loaders[AssetType_Shader].loader = &shader_loader;
	loaders[AssetType_Model].loader = &model_loader;
	loaders[AssetType_Sound].loader = &sound_loader;
	loaders[AssetType_Script].loader = &script_loader;

	loaders[AssetType_Texture].log_channel = logs::OpenChannel(log_channel, "TEXTURE");
	loaders[AssetType_Shader].log_channel = logs::OpenChannel(log_channel, "SHADER");
	loaders[AssetType_Model].log_channel = logs::OpenChannel(log_channel, "MODEL");
	loaders[AssetType_Sound].log_channel = logs::OpenChannel(log_channel, "SOUND");
	loaders[AssetType_Script].log_channel = logs::OpenChannel(log_channel, "SCRIPT");

	upload_queue.reserve(ASSETS_UPLOAD_QUEUE_MAX_SIZE);

	selected_system = this;

	DebugLogI(log_channel, "Initialized.");
}

ast::AssetSystem::~AssetSystem()
{
	{
		SpinLockScope scope(registry_lock);

		for (int i = 0; i < record_pool.LiveCount(); i++) {
			Record *record = &records[i];
			IAssetLoader *loader = loaders[record->type].loader;

			if (loader && record->load_state == LoadState_Ready)
				loader->DestroyAsset(&record->asset);
		}
	}

	for (Upload &upload : upload_queue)
		delete upload.result.user_data;

	for (LoadJobParam *param : load_params)
		delete param;

	delete[] records;

	DebugLogI(log_channel, "Destroyed.");

	if (selected_system == this)
		selected_system = nullptr;
}

ast::LoadJobParam *ast::AssetSystem::AllocLoadParam()
{
	LoadJobParam *param = new LoadJobParam();

	SpinLockScope scope(param_lock);
	load_params.push_back(param);

	return param;
}

void ast::AssetSystem::FreeLoadParam(LoadJobParam *param)
{
	{
		SpinLockScope scope(param_lock);

		for (u64 i = 0; i < load_params.size(); i++) {
			if (load_params[i] == param) {
				load_params[i] = load_params.back();
				load_params.pop_back();
				break;
			}
		}
	}

	delete param;
}

void ast::AssetSystem::SetFallback(Handle handle)
{
	loaders[handle.type].fallback = handle;
}

void ast::AssetSystem::PollHotReloads()
{
	SpinLockScope scope(registry_lock);

	for (int i = 0; i < record_pool.LiveCount(); i++) {
		Record *record = &records[i];

		if (record->load_state != LoadState_Ready)
			continue;

		String system_path = GetSystemFilePath(record->metadata.path);

		i64 latest_write = fs::GetLastWriteTime(system_path);

		if (latest_write > record->metadata.last_write_time) {
			record->metadata.last_write_time = latest_write;

			record->load_state = LoadState_Loading;

			record->is_reloading = true;

			LoadJobParam *param = AllocLoadParam();
			param->record = record;
			param->counter = nullptr;

			job::Decl decl;
			decl.entry_point = LoadJob;
			decl.param = param;
			decl.priority = job::Priority_Low;

			job::Kick(decl, nullptr);

			DebugLogD(
				log_channel,
				"Reloading: %s",
				record->metadata.path.c_str()
			);
		}
	}
}

void ast::AssetSystem::FlushUploads()
{
	Vector<Upload> pending;

	{
		SpinLockScope scope(upload_lock);

		if (upload_queue.empty())
			return;

		pending.swap(upload_queue);
		upload_queue.reserve(ASSETS_UPLOAD_QUEUE_MAX_SIZE);
	}

	gfx::Device *device = gfx::Device::Get();

	unsigned pending_count = pending.size();
	unsigned base = 0;

	while (base < pending_count) {
		u64 batch_stage_size = 0;
		u64 batch_count = 0;

		for (unsigned i = base; i < pending_count; i++) {
			u64 upload_size = pending[i].result.stage_size;
			u64 aligned_size = MemAlignUp(upload_size, 16);

			if (aligned_size > ASSETS_GPU_UPLOAD_CHUNK_MAX_SIZE && batch_stage_size == 0) {
				batch_stage_size = aligned_size;
				batch_count = 1;

				break;
			}

			if (batch_stage_size + aligned_size > ASSETS_GPU_UPLOAD_CHUNK_MAX_SIZE)
				break;

			batch_stage_size += aligned_size;
			batch_count++;
		}

		gfx::ResourceKey staging_buffer;

		if (batch_stage_size > 0)
			staging_buffer = device->StageAlloc(batch_stage_size);

		{
			gfx::CmdBuffer cmd = device->SubmitImBegin();

			u64 stage_offset = 0;

			for (unsigned i = 0; i < batch_count; i++) {
				Upload *upload = &pending[base + i];
				Record *record = upload->record;

				TypeLoader *type_loader = &loaders[record->type];
				IAssetLoader *loader = type_loader->loader;

				LoaderContext ctx;
				ctx.log_channel = type_loader->log_channel;
				ctx.metadata = record->metadata;
				ctx.current_residency = record->current_residency;
				ctx.target_residency = record->target_residency;

				if (upload->result.failed) {
					if (record->is_reloading) {
						record->is_reloading = false;
						record->load_state = LoadState_Ready;

						DebugLogW(
							log_channel,
							"Reload failed for %s, keeping previous version.",
							record->metadata.path.c_str()
						);
					} else if (record->is_streaming) {
						record->is_streaming = false;

						record->target_residency = record->current_residency;

						DebugLogW(
							log_channel,
							"Streaming failed for %s, keeping previous version.",
							record->metadata.path.c_str()
						);
					} else {
						record->load_state = LoadState_Failed;
					}
				} else {
					if (record->is_reloading)
						loader->DestroyAsset(&record->asset);

					record->asset = Asset();

					loader->Alloc(ctx, &upload->result, &record->asset);
					loader->UploadGPU(ctx, &upload->result, &record->asset, cmd, staging_buffer, stage_offset);
					loader->DestroyIntermediateResources(&upload->result);

					DebugLogD(
						log_channel,
						"Finalized: %s",
						record->metadata.path.c_str()
					);

					record->load_state = LoadState_Ready;

					record->is_reloading = false;
					record->is_streaming = false;

					record->current_residency = record->target_residency;

					stage_offset += MemAlignUp(upload->result.stage_size, 16);
				}

				delete upload->result.user_data;
				upload->result.user_data = nullptr;
			}

			device->SubmitImEnd(cmd);
		}

		if (!staging_buffer.IsNull())
			device->BufferDestroy(staging_buffer);

		base += batch_count;
	}
}

void ast::AssetSystem::DoAssetStreaming()
{
	SpinLockScope scope(registry_lock);

	for (int i = 0; i < record_pool.LiveCount(); i++) {
		Record *record = &records[i];

		IAssetLoader *loader = loaders[record->type].loader;

		if (!loader || !loader->IsStreamable())
			continue;

		int cmp = record->target_residency.Compare(record->current_residency);

		if (cmp > 0) {
			//record->load_state = LoadState_Loading;
			//record->is_streaming = true;

			DebugLogD(
				log_channel,
				"(not implemented) Streaming in/upgrading: %s",
				record->metadata.path.c_str()
			);
		} else if (cmp < 0) {
			DebugLogD(
				log_channel,
				"(not implemented) Streaming out/evicting: %s",
				record->metadata.path.c_str()
			);

			record->current_residency = record->target_residency;
		}
	}
}

void ast::AssetSystem::SetStreamingState(Handle handle, const LoadResidencyCtx &residency_ctx)
{
	Record *record = GetRecord(handle);

	if (!record)
		return;

	SpinLockScope scope(registry_lock);

	IAssetLoader *loader = loaders[record->type].loader;

	record->target_residency = loader->CalculateResidency(residency_ctx);

	// todo: calculate a priority as well?
	// maybe something like:
	//   record->streaming_priority = CalcStreamingPriority(residency_ctx, frames_since_last_used);
}

ast::AssetType ast::AssetSystem::GetAssetTypeFromPath(const String &path)
{
	u64 last = path.find_last_of('.');
	String extension = (last == String::npos) ? String() : path.substr(last + 1);

	for (unsigned i = AssetType_Null + 1; i < AssetType_MaxEnum; i++) {
		IAssetLoader *loader = loaders[i].loader;

		if (!loader)
			continue;

		for (const String &candidate : loader->GetFileExtensions()) {
			if (extension == candidate)
				return (AssetType)i;
		}
	}

	DebugLogB(log_channel, "Couldn't find any asset loader for extension: \".%s\"", extension.c_str());

	return AssetType_Null;
}

ast::Handle ast::AssetSystem::HandleFromFilePath(const String &path)
{
	SpinLockScope scope(registry_lock);

	Handle existing = PathMapFind(path);

	if (!existing.IsNull())
		return existing;

	AssetType type = GetAssetTypeFromPath(path);

	Record *record = AllocRecord(type);

	String sys_path = GetSystemFilePath(path);

	record->metadata.path = path;
	record->metadata.last_write_time = fs::GetLastWriteTime(sys_path);

	Handle handle;
	handle.id = record->id;
	handle.generation = record->generation;
	handle.type = type;

	PathMapInsert(path, handle);

	return handle;
}

ast::Asset *ast::AssetSystem::GetOrFallback(Handle handle)
{
	Record *record = GetRecord(handle);

	if (record && record->load_state == LoadState_Ready)
		return &record->asset;

	const char *type_string = StringFromType(handle.type);

	if (record) {
		DebugLogW(
			log_channel,
			"%s asset not found (path: %s). Falling back...",
			type_string,
			record->metadata.path.c_str()
		);
	} else {
		DebugLogW(
			log_channel,
			"%s asset not found. Falling back...",
			type_string
		);
	}

	Record *fallback = GetRecord(loaders[handle.type].fallback);

	if (!fallback) {
		DebugLogB(log_channel, "No fallback found for %s. We're fucked.", type_string);
		return nullptr;
	}

	return &fallback->asset;
}

ast::Asset *ast::AssetSystem::GetOrBreak(Handle handle)
{
	Record *record = GetRecord(handle);

	if (!record)
		DebugLogB(log_channel, "Asset not found.");
	else if (record->load_state != LoadState_Ready)
		DebugLogB(log_channel, "Asset not ready.");
	else
		return &record->asset;

	return nullptr;
}

ast::LoadState ast::AssetSystem::GetLoadState(Handle handle)
{
	Record *record = GetRecord(handle);
	return record ? record->load_state : LoadState_Unloaded;
}

void ast::AssetSystem::LoadJob(void *param)
{
	AssetSystem *self = selected_system;
	LoadJobParam *load_params = (LoadJobParam *)param;

	AssetType type = load_params->record->type;

	LoaderContext ctx;
	ctx.log_channel = self->loaders[type].log_channel;
	ctx.metadata = load_params->record->metadata;
	ctx.current_residency = load_params->record->current_residency;
	ctx.target_residency = load_params->record->target_residency;

	IAssetLoader *loader = self->loaders[type].loader;

	LoadResult result = loader->Load(ctx);

	if (result.failed) {
		DebugLogD(
			self->log_channel,
			"Failed to load: %s",
			ctx.metadata.path.c_str()
		);
	} else {
		DebugLogD(
			self->log_channel,
			"Loaded: %s",
			ctx.metadata.path.c_str()
		);

		Vector<job::Decl> decls;

		for (const Handle &dependency : result.dependencies) {
			Record *record = self->GetRecord(dependency);

			SpinLockScope scope(self->registry_lock);

			if (record->load_state == LoadState_Unloaded) {
				record->load_state = LoadState_Loading;

				LoadJobParam *dep_param = self->AllocLoadParam();
				dep_param->record = record;
				dep_param->counter = load_params->counter;

				job::Decl decl;
				decl.entry_point = LoadJob;
				decl.param = dep_param;
				decl.priority = job::Priority_Normal;
				decl.flags = job::Flag_None;

				decls.push_back(decl);
			}
		}

		if (!decls.empty())
			job::Batch(decls.data(), decls.size(), load_params->counter);
	}

	Upload upload;
	upload.record = load_params->record;
	upload.result = result;

	{
		SpinLockScope scope(self->upload_lock);
		self->upload_queue.push_back(upload);
	}

	self->FreeLoadParam(load_params);
}

ast::Handle ast::AssetSystem::RequireAsset(const String &path, job::Counter *counter)
{
	Handle handle = HandleFromFilePath(path);
	Record *record = GetRecord(handle);

	{
		SpinLockScope scope(registry_lock);

		if (record->load_state != LoadState_Unloaded)
			return handle;

		record->load_state = LoadState_Loading;
	}

	LoadJobParam *param = AllocLoadParam();
	param->record = record;
	param->counter = counter;

	job::Decl decl;
	decl.entry_point = LoadJob;
	decl.param = param;
	decl.priority = job::Priority_Normal;
	decl.flags = job::Flag_None;

	job::Kick(decl, counter);

	return handle;
}

ast::Handle ast::AssetSystem::RequireAssetBlocking(const String &path)
{
	job::Counter counter;
	Handle handle = RequireAsset(path, &counter);
	WaitForLoad(&counter);
	return handle;
}

void ast::AssetSystem::DestroyAsset(Handle handle)
{
	Record *record = GetRecord(handle);

	if (!record)
		return;

	SpinLockScope scope(registry_lock);

	IAssetLoader *loader = loaders[record->type].loader;

	if (record->load_state == LoadState_Ready)
		loader->DestroyAsset(&record->asset);

	String path = record->metadata.path;

	FreeRecord(record);

	PathMapRemove(path);
}

void ast::AssetSystem::WaitForLoad(job::Counter *counter)
{
	counter->Yield(0);
	FlushUploads();
}

void ast::AssetSystem::AddShaderSearchPath(const String &directory)
{
	shader_search_paths.push_back(directory);
}

const Vector<String> &ast::AssetSystem::GetShaderSearchPaths() const
{
	return shader_search_paths;
}

void ast::AssetSystem::Mount(const String &prefix, const String &directory)
{
	DebugLogAssert(log_channel, !directory.empty(), "Directory length must be greater than zero.");

	MountPoint mp;
	mp.prefix = prefix;
	mp.directory = directory;

	mount_points.push_back(mp);
}

String ast::AssetSystem::GetSystemFilePath(const String &path) const
{
	const MountPoint *best = nullptr;
	u64 best_len = 0;

	for (const MountPoint &m : mount_points) {
		if (str::StartsWith(path, m.prefix) && m.prefix.size() > best_len) {
			best = &m;
			best_len = m.prefix.size();
		}
	}

	if (!best) {
		DebugLogB(
			log_channel,
			"Failed to find system file path for asset path: \"%s\"",
			path.c_str()
		);

		return path;
	}

	String suffix = path.substr(best_len);

	return path::Join(best->directory, suffix);
}

ast::AssetSystem *ast::AssetSystem::Get()
{
	return selected_system;
}

ast::Handle ast::AssetSystem::PathMapFind(const String &path)
{
	auto it = path_map.find(path);

	if (it == path_map.end())
		return Handle();

	return it->second;
}

void ast::AssetSystem::PathMapInsert(const String &path, Handle handle)
{
	path_map[path] = handle;
}

void ast::AssetSystem::PathMapRemove(const String &path)
{
	path_map.erase(path);
}

ast::Record *ast::AssetSystem::AllocRecord(AssetType type)
{
	unsigned id = record_pool.GetStableID();
	Record *record = &records[record_pool.IndexFromID(id)];

	unsigned next_gen = record->generation + 1;

	*record = Record();

	record->id = id;
	record->generation = next_gen;
	record->type = type;

	return record;
}

void ast::AssetSystem::FreeRecord(Record *record)
{
	unsigned dense = record_pool.IndexFromID(record->id);
	unsigned moved = record_pool.FreeID(record->id);

	if (dense != moved)
		records[dense] = std::move(records[moved]);
}

ast::Record *ast::AssetSystem::GetRecord(Handle handle)
{
	if (handle.IsNull())
		return nullptr;

	SpinLockScope scope(registry_lock);

	Record *record = &records[record_pool.IndexFromID(handle.id)];

	if (record->generation == handle.generation)
		return record;

	return nullptr;
}

void ast::AssetSystem::MarkTextureAsSRGB(Handle handle)
{
	if (handle.IsNull() || handle.type != AssetType_Texture)
		return;

	Record *record = GetRecord(handle);
	if (!record)
		return;

	SpinLockScope scope(registry_lock);
	record->metadata.srgb = true;
}
