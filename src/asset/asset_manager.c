
internal u64
AST_FindSchemeSeparator(String8 path)
{
	AssertTrue(path.len >= 2);
	
	for (u64 i = 0; i < path.len - 2; i++)
	{
		if (path.str[i]     == ':' &&
			path.str[i + 1] == '/' &&
			path.str[i + 2] == '/')
		{
			return i;
		}
	}

	return path.len;
}

internal u32
AST_AllocSlot(AST_Assets *assets)
{
	if (assets->free_index_count > 0)
		return assets->free_indices[--assets->free_index_count];

	AssertTrue(assets->record_count < ArraySize(assets->records));

	return assets->record_count++;
}

internal void
AST_FreeSlot(AST_Assets *assets, u32 index)
{
	AssertTrue(assets->free_index_count < ArraySize(assets->free_indices));

	assets->free_indices[assets->free_index_count] = index;
	assets->free_index_count++;
}

internal AST_Handle
AST_AllocRecord(AST_Assets *assets, String8 path)
{
	u32 index = AST_AllocSlot(assets);

	AST_Record *record = &assets->records[index];
	MemZeroStruct(record);

	record->path = String8Clone(assets->arena, path);
	record->state = AST_State_Unloaded;
	
	record->generation++;

	AST_Handle handle = {0};
	handle.index = index;
	handle.generation = record->generation;

	return handle;
}

internal AST_Record *
AST_GetRecord(AST_Assets *assets, AST_Handle handle)
{
	AssertTrue(AST_IsValid(assets, handle));
	return &assets->records[handle.index];
}

internal const AST_Record *
AST_GetRecordConst(const AST_Assets *assets, AST_Handle handle)
{
	AssertTrue(AST_IsValid(assets, handle));
	return &assets->records[handle.index];
}

internal AST_Handle
AST_PathMapFind(const AST_Assets *assets, String8 path)
{
	// shitty but cool trick but i really shouldnt be doing this lol
	// note: AST_MANAGER_MAX_RECORDS *must* be a power of two for this to work.
	
	const u64 hash = HashStr8(path);
	const u32 mask = AST_MANAGER_MAX_RECORDS - 1;

	for (u32 i = 0; i < AST_MANAGER_MAX_RECORDS; i++)
	{
		u32 idx = (u32)((hash + i) & mask);

		const AST_PathMapEntry *entry = &assets->path_map[idx];
		
		if (!entry->occupied)
			break;

		if (String8Match(entry->path, path))
			return entry->value;
	}

	return AST_HandleNull();
}

internal void
AST_PathMapInsert(AST_Assets *assets, String8 path, AST_Handle handle)
{
	AssertTrue(AST_IsValid(assets, handle));

	const u64 hash = HashStr8(path);
	const u32 mask = AST_MANAGER_MAX_RECORDS - 1;

	for (u32 i = 0; i < AST_MANAGER_MAX_RECORDS; i++)
	{
		u32 idx = (u32)((hash + i) & mask);

		AST_PathMapEntry *entry = &assets->path_map[idx];
		
		if (!entry->occupied)
		{
			entry->occupied = true;
			
			entry->value = handle;
			entry->path = String8Clone(assets->arena, path);
			
			return;
		}
	}

	AssertTrue(false && "Cannot add more paths to path map in Assets.");
}

internal u32
AST_LoadArenaAcquire(AST_Assets *assets)
{
	osapi->SpinLockAcquire(&assets->load_arena_spinlock);
 
	AssertTrue(assets->free_load_arena_count > 0 && "Load arena pool exhausted.");
 
	u32 index = assets->free_load_arenas[--assets->free_load_arena_count];
 
	osapi->SpinLockRelease(&assets->load_arena_spinlock);
 
	return index;
}

internal void
AST_LoadArenaRelease(AST_Assets *assets, u32 index)
{
	ArenaClear(&assets->load_arenas[index]);
 
	osapi->SpinLockAcquire(&assets->load_arena_spinlock);
 
	assets->free_load_arenas[assets->free_load_arena_count] = index;
	assets->free_load_arena_count++;
 
	osapi->SpinLockRelease(&assets->load_arena_spinlock);
}

internal void
AST_Init(AST_Assets *assets, Arena *arena,
		 GFX_Device *device,
		 const AUD_BackendAPI *audio_backend)
{
	MemZeroStruct(assets);

	assets->arena = arena;

	assets->device = device;
	assets->audio_backend = audio_backend;
	
	assets->async_upload_counter = osapi->JobCounterAlloc(arena, 0);

	assets->upload_mutex     = osapi->MutexCreate();
	assets->dependency_mutex = osapi->MutexCreate();
	assets->allocation_mutex = osapi->MutexCreate();
	assets->loading_mutex    = osapi->MutexCreate();
	assets->loading_cond     = osapi->CondVarCreate();

	for (u32 i = 0; i < AST_LOAD_ARENA_COUNT; i++)
	{
		assets->load_arenas[i] = ArenaInitArena(arena, AST_LOAD_ARENA_SIZE);
		assets->free_load_arenas[i] = i;
	}

	assets->free_load_arena_count = AST_LOAD_ARENA_COUNT;

#define AssetDef(name) assets->serializers[AST_Type_##name] = AST_Get##name##Serializer();
#include "asset_definitions.inc"
#undef AssetDef
	
	DebugLogF("Assets Initialized.");
}

internal void
AST_Destroy(AST_Assets *assets)
{
	for (u32 i = 0; i < assets->record_count; i++)
	{
		AST_Record *record = &assets->records[i];

		if (record->state == AST_State_Ready)
		{
			AST_Serializer *s = &assets->serializers[record->asset.type];

			if (s->Dispose)
				s->Dispose(&record->asset, assets);
		}
	}
	
	osapi->MutexDestroy   (assets->upload_mutex);
	osapi->MutexDestroy   (assets->dependency_mutex);
	osapi->MutexDestroy   (assets->allocation_mutex);
	osapi->MutexDestroy   (assets->loading_mutex);
	osapi->CondVarDestroy (assets->loading_cond);

	DebugLogF("Assets Destroyed.");
}

internal void
AST_Mount(AST_Assets *assets, String8 prefix, String8 directory)
{
	AssertTrue(directory.len > 0);
	AssertTrue((directory.str[directory.len - 1] == '/') ||
			   (directory.str[directory.len - 1] == '\\'));
	
	AssertTrue(assets->mount_point_count < ArraySize(assets->mount_points));

	AST_MountPoint *mp = &assets->mount_points[assets->mount_point_count++];
	mp->prefix    = String8Clone(assets->arena, prefix);
	mp->directory = String8Clone(assets->arena, directory);
}

internal String8
AST_GetSystemFilePath(AST_Assets *assets, Arena *arena, String8 path)
{
	u64 sep = AST_FindSchemeSeparator(path);

	if (sep < path.len)
	{
		String8 prefix   = String8Init(path.str, sep);
		String8 relative = String8Init(path.str + sep + 3, path.len - sep - 3);

		for (u32 i = 0; i < assets->mount_point_count; i++)
		{
			if (String8Match(prefix, assets->mount_points[i].prefix))
				return String8Append(arena, assets->mount_points[i].directory, relative);
		}
	}

	DebugLogF("Unrecognised asset prefix in path: \"%.*s\"", (int)path.len, path.str);
	AssertTrue(false);

	return path;
}

internal b32
AST_IsLoaded(const AST_Assets *assets, AST_Handle handle)
{
	return AST_StateIsLoaded(AST_GetRecordConst(assets, handle)->state);
}

internal b32
AST_IsLoading(const AST_Assets *assets, AST_Handle handle)
{
	return AST_StateIsLoading(AST_GetRecordConst(assets, handle)->state);
}

internal b32
AST_IsValid(const AST_Assets *assets, AST_Handle handle)
{
	return (handle.index < assets->record_count &&
			assets->records[handle.index].generation == handle.generation);
}

internal void
AST_LoadNow(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	AST_Record *record = AST_GetRecord(assets, handle);

	if (AST_StateNeedsLoad(record->state))
	{
		ScratchArena scratch = ScratchBegin(&assets->arena, 1);
	
		JOB_Counter *counter = osapi->JobCounterAlloc(scratch.arena, 0);

		AST_Load(assets, handle, type, counter);

		osapi->JobYield(counter, 0);

		while (record->state != AST_State_Ready ||
			   record->state == AST_State_Failed)
		{
			AST_ResolvePendingDependencies(assets, counter);
			osapi->JobYield(counter, 0);
			AST_FlushUploads(assets);
		}
	
		ScratchRelease(&scratch);
	}
}

internal void
AST_LoadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	AST_Record *record = AST_GetRecord(assets, handle);

	if (AST_StateNeedsLoad(record->state))
		AST_Load(assets, handle, type, assets->async_upload_counter);
}

internal void
AST_ReloadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	AST_Record *record = AST_GetRecord(assets, handle);

	if (!AST_StateIsLoading(record->state))
		AST_Load(assets, handle, type, assets->async_upload_counter);
}

JOB_ENTRY_POINT_DEF(AST_LoadJobEntry)
{
	AST_LoadJobParam *load_params = param;

	u32 arena_index = AST_LoadArenaAcquire(load_params->assets);
	Arena *load_arena = &load_params->assets->load_arenas[arena_index];
	
	AST_Context ctx = {0};
	ctx.scope = load_arena;
	ctx.assets = load_params->assets;
	ctx.metadata = load_params->metadata;

	AST_Serializer *serializer = &load_params->assets->serializers[load_params->type];

	AST_SerializerPipelineData load_data = serializer->Cpu(&ctx);

	if (load_data.failed)
	{
		DebugLogF("Failed to load asset: %.*s", (int)ctx.metadata.path.len, ctx.metadata.path.str);
		AssertTrue(false);
	}

	AST_Upload upload = {0};
	upload.load_arena_index = arena_index;
	upload.metadata = load_params->metadata;
	upload.handle = load_params->handle;
	upload.type = load_params->type;
	upload.load_data = load_data;
 
	osapi->MutexLock(load_params->assets->dependency_mutex);
	AST_UploadQueuePush(&load_params->assets->dependency_queue, &upload);
	osapi->MutexUnlock(load_params->assets->dependency_mutex);
}

internal void
AST_Load(AST_Assets *assets, AST_Handle handle, AST_Type type, JOB_Counter *counter)
{
	assets->records[handle.index].state = AST_State_CpuStage;

	// params get dumped onto the permanent arena which isnt that big a deal
	// 'cuz they're only like a couple of bytes so whatever.
	osapi->MutexLock(assets->allocation_mutex);
	AST_LoadJobParam *params = ArenaPushArray(assets->arena, AST_LoadJobParam, 1);
	osapi->MutexUnlock(assets->allocation_mutex);

	AST_MetaData metadata = {0};
	metadata.path = AST_GetRecord(assets, handle)->path;
	
	params->assets   = assets;
	params->handle   = handle;
	params->type     = type;
	params->metadata = metadata;
 
	JOB_Decl decl = {0};
	decl.EntryPoint = AST_LoadJobEntry;
	decl.priority = JOB_Priority_Normal;
	decl.param = params;
 
	osapi->JobKick(&decl, counter);
}

internal void
AST_NotifyDependents(AST_Assets *assets, AST_Handle handle)
{
	osapi->MutexLock(assets->dependency_mutex);
	AST_NotifyDependentsNoLock(assets, handle, false);
	osapi->MutexUnlock(assets->dependency_mutex);
}

internal void
AST_NotifyDependentsNoLock(AST_Assets *assets, AST_Handle handle, b32 failed)
{
	AssertTrue(AST_IsValid(assets, handle));

	AST_Record *record = AST_GetRecord(assets, handle);

	for (u32 i = 0; i < record->dependent_count; i++)
	{
		AST_Handle  parent_handle = record->dependents[i];
		AST_Record *parent_record = AST_GetRecord(assets, parent_handle);

		if (failed)
		{
			parent_record->state = AST_State_Failed;
			AST_NotifyDependentsNoLock(assets, parent_handle, true);
		}
		else
		{
			if (parent_record->hot_pending_dependencies > 0)
				parent_record->hot_pending_dependencies--;

			if (parent_record->hot_pending_dependencies == 0 &&
				parent_record->state == AST_State_WaitingForDependencies)
			{
				parent_record->state = AST_State_GpuStage;

				osapi->MutexLock(assets->upload_mutex);
				AST_UploadQueuePush(&assets->upload_queue, &parent_record->stashed_upload);
				osapi->MutexUnlock(assets->upload_mutex);
			}
		}
	}

	record->dependent_count = 0;
}

internal void
AST_ResolvePendingDependencies(AST_Assets *assets, JOB_Counter *counter)
{
	osapi->MutexLock(assets->dependency_mutex);

	AST_UploadQueue *q = &assets->dependency_queue;

	for (u32 i = 0; i < q->count; i++)
	{
		AST_Upload *upload = &q->elements[i];
		AST_Record *record = AST_GetRecord(assets, upload->handle);

		if (upload->load_data.failed)
		{
			record->state = AST_State_Failed;

			AST_NotifyDependentsNoLock(assets, upload->handle, true);

			continue;
		}

		u32 unresolved = 0;

		for (u32 j = 0; j < upload->load_data.dependency_count; j++)
		{
			AST_Handle  dep_handle = upload->load_data.dependencies[j];
			AST_Record *dep_record = AST_GetRecord(assets, dep_handle);

			if (!AST_StateIsLoading(dep_record->state) && AST_StateNeedsLoad(dep_record->state))
			{
				AST_Load(assets, dep_handle, AST_Type_Texture, counter); // TODO: infer type from dependency !!!
			}

			if (!AST_StateIsFinalized(dep_record->state))
			{
				unresolved++;

				AssertTrue(dep_record->dependent_count < ArraySize(dep_record->dependents));

				dep_record->dependents[dep_record->dependent_count] = upload->handle;
				dep_record->dependent_count++;
			}
		}

		if (unresolved > 0)
		{
			record->state = AST_State_WaitingForDependencies;
			record->hot_pending_dependencies = unresolved;
			record->stashed_upload = *upload;
		}
		else
		{
			record->state = AST_State_GpuStage;

			osapi->MutexLock(assets->upload_mutex);
			AST_UploadQueuePush(&assets->upload_queue, upload);
			osapi->MutexUnlock(assets->upload_mutex);
		}
	}

	AST_UploadQueueClear(q);

	osapi->MutexUnlock(assets->dependency_mutex);
}

internal void
AST_PollHotReloads(AST_Assets *assets)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
 
	for (u32 i = 0; i < assets->record_count; i++)
	{
		AST_Record *record = &assets->records[i];
 
		if (record->state != AST_State_Ready)
			continue;
 
		String8 sys_path = AST_GetSystemFilePath(assets, scratch.arena, record->path);
		u64 newest_write = osapi->GetFileLastWriteTime(sys_path);
 
		for (u32 j = 0; j < record->watch_path_count; j++)
		{
			u64 t = osapi->GetFileLastWriteTime(record->watch_paths[j]);
 
			if (t > newest_write)
				newest_write = t;
		}
 
		if (newest_write > record->last_write_time && newest_write != 0)
		{
			record->last_write_time = newest_write;
 
			AST_Handle handle = record->asset.handle;
			AST_Type type = record->asset.type;
 
			AST_ReloadAsync(assets, handle, type);
		}
 
		ScratchClear(&scratch);
	}
 
	ScratchRelease(&scratch);
}

internal void
AST_FlushUploads(AST_Assets *assets)
{
	AST_ResolvePendingDependencies(assets, assets->async_upload_counter);

	osapi->MutexLock(assets->upload_mutex);

	if (assets->upload_queue.count == 0)
	{
		osapi->MutexUnlock(assets->upload_mutex);
		return;
	}

	AST_UploadQueue pending = assets->upload_queue;
	AST_UploadQueueClear(&assets->upload_queue);

	osapi->MutexUnlock(assets->upload_mutex);

	u32 base = 0;

	while (base < pending.count)
	{
		u64 batch_stage_size = 0;
		u64 batch_count = 0;

		for (u32 i = base; i < pending.count; i++)
		{
			u64 upload_size  = pending.elements[i].load_data.stage_size;
			u64 aligned_size = MemAlignUp(upload_size, 16);

			if (aligned_size > AST_GPU_UPLOAD_CHUNK && batch_stage_size == 0)
			{
				batch_stage_size = aligned_size;
				batch_count = 1;
				
				break;
			}

			if (batch_stage_size + aligned_size > AST_GPU_UPLOAD_CHUNK)
				break;

			batch_stage_size += aligned_size;
			batch_count++;
		}

		GFX_BufferKey staging_buffer = GFX_BufferKeyNull();

		if (batch_stage_size > 0)
			staging_buffer = GFX_DeviceStageAlloc(assets->device, batch_stage_size);

		GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(assets->device);
		{
			u64 stage_offset = 0;

			for (u32 i = 0; i < batch_count; i++)
			{
				AST_Upload *upload = &pending.elements[base + i];
				AST_Record *record = AST_GetRecord(assets, upload->handle);

				AST_Serializer *serializer = &assets->serializers[upload->type];

				AST_Asset *asset = &record->asset;

				AST_Context ctx = {0};
				ctx.scope = &assets->load_arenas[upload->load_arena_index];
				ctx.assets = assets;
				ctx.metadata = upload->metadata;

				if (upload->load_data.failed)
				{
					record->state = AST_State_Failed;
					// TODO: assign fallback / placeholder asset.
				}
				else
				{
					b32 is_new = asset->type == AST_Type_Unknown;

					DebugLogF("%s %.*s...",
							  is_new ? "Creating" : "Reloading",
							  (int)upload->metadata.path.len,
							  upload->metadata.path.str);

					asset->type = upload->type;
					asset->handle = upload->handle;

					if (is_new)
					{
						serializer->Alloc(&ctx, &upload->load_data, asset);
					}
					else
					{
						serializer->Reload(&ctx, &upload->load_data, asset);
					}
					
					if (serializer->Gpu)
					{
						GFX_Buffer *gfx_staging_buffer = GFX_DeviceBufferFromKey(assets->device, staging_buffer);
						serializer->Gpu(&ctx, &upload->load_data, asset, &cmd, gfx_staging_buffer, stage_offset);
					}

					if (serializer->End)
					{
						serializer->End(&upload->load_data);
					}

					if (upload->load_data.watch_path_count > 0)
					{
						osapi->MutexLock(assets->allocation_mutex);

						record->watch_path_count = upload->load_data.watch_path_count;
						record->watch_paths = ArenaPushArray(assets->arena, String8, record->watch_path_count);

						for (u32 j = 0; j < record->watch_path_count; j++)
							record->watch_paths[j] = String8Clone(assets->arena, upload->load_data.watch_paths[j]);

						osapi->MutexUnlock(assets->allocation_mutex);
					}

					ScratchArena scratch = ScratchBegin(NULL, 0);
					{
						String8 sys_path = AST_GetSystemFilePath(assets, scratch.arena, record->path);
						record->last_write_time = osapi->GetFileLastWriteTime(sys_path);
 
						for (u32 j = 0; j < record->watch_path_count; j++)
						{
							u64 t = osapi->GetFileLastWriteTime(record->watch_paths[j]);
 
							if (t > record->last_write_time)
								record->last_write_time = t;
						}
					}
					ScratchRelease(&scratch);

					record->state = AST_State_Ready;

					AST_NotifyDependents(assets, upload->handle);

					stage_offset += MemAlignUp(upload->load_data.stage_size, 16);
				}
				
				AST_LoadArenaRelease(assets, upload->load_arena_index);
			}
		}
		GFX_DeviceSubmitImEnd(assets->device, &cmd);

		if (!GFX_BufferKeyIsNull(staging_buffer))
			GFX_DeviceBufferDestroy(assets->device, staging_buffer);

		base += batch_count;
	}

	osapi->CondVarBroadcast(assets->loading_cond);
}

internal void
AST_WaitForAsyncUploads(AST_Assets *assets)
{
	osapi->JobYield(assets->async_upload_counter, 0);
}

internal AST_Handle
AST_FromFilePath(AST_Assets *assets, String8 path)
{
	AST_Handle existing = AST_PathMapFind(assets, path);

	if (!AST_HandleIsNull(existing))
		return existing;

	osapi->MutexLock(assets->allocation_mutex);
	AST_Handle handle = AST_AllocRecord(assets, path);
	osapi->MutexUnlock(assets->allocation_mutex);

	AST_PathMapInsert(assets, path, handle);

	return handle;
}

internal AST_Handle
AST_Require(AST_Assets *assets, String8 path, AST_Type type)
{
	AST_Handle handle = AST_FromFilePath(assets, path);

	AST_Record *record = AST_GetRecord(assets, handle);

	if (AST_StateNeedsLoad(record->state))
		AST_LoadAsync(assets, handle, type);

	return handle;
}
