
static A_State *a_assets = NULL;

internal A_Handle A_PathMapFind(String8 path)
{
	for (A_PathMapEntry *entry = a_assets->path_entry_sentinel.next;
		 entry != &a_assets->path_entry_sentinel;
		 entry = entry->next)
	{
		if (String8Match(entry->path, path))
			return entry->handle;
	}
	
	return A_HandleNull();
}

internal void A_PathMapInsert(String8 path, A_Handle handle)
{
	A_PathMapEntry *entry = NULL;

	if (a_assets->path_free_sentinel.next != &a_assets->path_free_sentinel)
	{
		entry = a_assets->path_free_sentinel.next;

		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;

		MemZeroStruct(entry);
	}
	else
	{
		entry = ArenaPushArray(a_assets->arena, A_PathMapEntry, 1);
	}

	entry->path = path;
	entry->handle = handle;
	
	entry->next = a_assets->path_entry_sentinel.next;
	entry->prev = &a_assets->path_entry_sentinel;

	entry->next->prev = entry;
	entry->prev->next = entry;
}

internal void A_PathMapRemove(String8 path)
{
	for (A_PathMapEntry *entry = a_assets->path_entry_sentinel.next;
		 entry != &a_assets->path_entry_sentinel;
		 entry = entry->next)
	{
		if (String8Match(path, entry->path))
			continue;
	
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;

		entry->next = a_assets->path_free_sentinel.next;
		entry->prev = &a_assets->path_free_sentinel;

		entry->next->prev = entry;
		entry->prev->next = entry;

		return;
	}
}

internal A_Record *A_AllocRecord(A_Type type)
{
	u32 id = DensePoolGetStableID(&a_assets->record_pool);
	A_Record *record = &a_assets->records[DensePoolIndexFromID(&a_assets->record_pool, id)];

	u32 next_gen = record->generation + 1;

	MemZeroStruct(record);
	
	record->id = id;
	record->generation = next_gen;
	record->type = type;
	
	record->arena = ArenaAlloc(A_ASSET_BACKING_ARENA_DEFAULT_SIZE);
	
	return record;
}

internal void A_FreeRecord(A_Record *record)
{
	ArenaRelease(&record->arena);
	
	u32 dense = DensePoolIndexFromID(&a_assets->record_pool, record->id);
	u32 moved = DensePoolFreeID(&a_assets->record_pool, record->id);

	a_assets->records[dense] = a_assets->records[moved];
}

internal A_Record *A_GetRecord(A_Handle handle)
{
	if (A_HandleIsNull(handle))
		return NULL;

	osapi->SpinLockAcquire(&a_assets->registry_spinlock);

	A_Record *found = NULL;
	
	A_Record *record = &a_assets->records[DensePoolIndexFromID(&a_assets->record_pool, handle.id)];
	
	if (record->generation == handle.generation)
		found = record;
	
	osapi->SpinLockRelease(&a_assets->registry_spinlock);
	
	return found;
}

internal void A_InitAndSelect(A_State *state, Arena *arena, LOG_Channel log_channel)
{
	state->arena = arena;
	state->log_channel = log_channel;

	DensePoolInit(&state->record_pool, arena, ArraySize(state->records));

#define AssetDef(name, upper)											\
	state->loaders[A_Type_##name].api = A_Get##name##LoaderAPI();		\
	state->loaders[A_Type_##name].log_channel = osapi->LogChannelOpenFrom(log_channel, String8Lit(STRINGIFY(upper))); \
	state->loaders[A_Type_##name].fallback = A_HandleNull();			\
	if (state->loaders[A_Type_##name].api.is_streamable)				\
	{																	\
		DebugLogAssert(state->loaders[A_Type_##name].log_channel, state->loaders[A_Type_##name].api.CalculateResidency, "Must have CalculateResidency implemented to support streaming."); \
	}
#include "asset_xmacro.inc"
#undef AssetDef

	state->path_entry_sentinel.next = &state->path_entry_sentinel;
	state->path_entry_sentinel.prev = &state->path_entry_sentinel;

	state->path_free_sentinel.next = &state->path_free_sentinel;
	state->path_free_sentinel.prev = &state->path_free_sentinel;
	
	A_SelectContext(state);
	
	DebugLogI(state->log_channel, "Initialized.");
}

internal void A_Destroy(void)
{
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
	
	for (u32 i = 0; i < DensePoolLiveCount(&a_assets->record_pool); i++)
	{
		A_Record *record = &a_assets->records[i];
		A_LoaderAPI *api = &a_assets->loaders[record->type].api;

		if (api->DestroyAsset)
			api->DestroyAsset(&record->asset);

		ArenaRelease(&record->arena);
	}

	osapi->SpinLockRelease(&a_assets->registry_spinlock);

	DebugLogI(a_assets->log_channel, "Destroyed.");

	a_assets = NULL;
}

internal void A_SelectContext(A_State *state)
{
	a_assets = state;
}

internal void A_SetFallback(A_Handle handle)
{
	a_assets->loaders[handle.type].fallback = handle;
}

internal void A_PollHotReloads(void)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
	
	for (u32 i = 0; i < DensePoolLiveCount(&a_assets->record_pool); i++)
	{
		A_Record *record = &a_assets->records[i];

		if (record->load_state != A_LoadState_Ready)
			continue;

		String8 system_path = A_GetSystemFilePath(scratch.arena, record->metadata.path);

		u64 latest_write = osapi->GetFileLastWriteTime(system_path);

		if (latest_write > record->metadata.last_write_time)
		{
			record->metadata.last_write_time = latest_write;

			ArenaReset(&record->arena);

			record->load_state = A_LoadState_Loading;

			record->is_reloading = true;
			
			A_LoadJobParam *param = ArenaPushArray(a_assets->arena, A_LoadJobParam, 1);
			param->record = record;
			param->counter = osapi->JobCounterAlloc(0);

			J_Decl decl = {0};
			decl.EntryPoint = A_LoadJob;
			decl.param = param;
			decl.priority = J_Priority_Low;

			osapi->JobKick(&decl, param->counter);
			
			DebugLogD(a_assets->log_channel,
					  "Reloading: %.*s",
					  String8VArg(record->metadata.path));
		}
		
		ScratchClear(&scratch);
	}

	osapi->SpinLockRelease(&a_assets->registry_spinlock);
	
	ScratchRelease(&scratch);
}

internal void A_FlushUploads(void)
{
	osapi->SpinLockAcquire(&a_assets->upload_spinlock);

	if (a_assets->upload_count == 0)
	{
		osapi->SpinLockRelease(&a_assets->upload_spinlock);
		return;
	}
	
	A_Upload pending[A_UPLOAD_QUEUE_MAX_SIZE] = {0};
	MemCopy(pending, a_assets->upload_queue, sizeof(pending));

	u32 pending_count = a_assets->upload_count;
	a_assets->upload_count = 0;
	
	osapi->SpinLockRelease(&a_assets->upload_spinlock);

	u32 base = 0;

	while (base < pending_count)
	{
		u64 batch_stage_size = 0;
		u64 batch_count = 0;

		for (u32 i = base; i < pending_count; i++)
		{
			u64 upload_size = pending[i].result.stage_size;
			u64 aligned_size = MemAlignUp(upload_size, 16);

			if (aligned_size > A_GPU_UPLOAD_CHUNK_MAX_SIZE && batch_stage_size == 0)
			{
				batch_stage_size = aligned_size;
				batch_count = 1;
				
				break;
			}

			if (batch_stage_size + aligned_size > A_GPU_UPLOAD_CHUNK_MAX_SIZE)
				break;

			batch_stage_size += aligned_size;
			batch_count++;
		}

		G_ResourceKey staging_buffer = G_ResourceKeyNull();

		if (batch_stage_size > 0)
			staging_buffer = G_StageAlloc(batch_stage_size);

		{
			G_CmdBuffer cmd = G_SubmitImBegin();
		
			u64 stage_offset = 0;

			for (u32 i = 0; i < batch_count; i++)
			{
				A_Upload *upload = &pending[base + i];
				A_Record *record = upload->record;

				A_TypeLoader *loader = &a_assets->loaders[record->type];

				A_LCTX ctx = {0};
				ctx.log_channel = loader->log_channel;
				ctx.metadata = record->metadata;
				ctx.current_residency = record->current_residency;
				ctx.target_residency = record->target_residency;

				if (upload->result.failed)
				{
					if (record->is_reloading)
					{
						record->is_reloading = false;
						
						DebugLogW(a_assets->log_channel,
								  "Reload failed for %.*s, keeping previous version.",
								  String8VArg(record->metadata.path));
					}
					else if (record->is_streaming)
					{
						record->is_streaming = false;

						record->target_residency = record->current_residency;
						
						DebugLogW(a_assets->log_channel,
								  "Streaming failed for %.*s, keeping previous version.",
								  String8VArg(record->metadata.path));
					}
					else
					{
						record->load_state = A_LoadState_Failed;
					}
				}
				else
				{
					if (loader->api.Alloc)
						loader->api.Alloc(&ctx, &upload->result, &record->arena, &record->asset);
					
					if (loader->api.UploadGPU)
						loader->api.UploadGPU(&ctx, &upload->result, &record->asset, &cmd, staging_buffer, stage_offset);

					if (loader->api.DestroyIntermediateResources)
						loader->api.DestroyIntermediateResources(&upload->result);

					DebugLogD(a_assets->log_channel,
							  "Finalized: %.*s",
							  String8VArg(record->metadata.path));
					
					record->load_state = A_LoadState_Ready;

					record->is_reloading = false;
					record->is_streaming = false;

					record->current_residency = record->target_residency;

					stage_offset += MemAlignUp(upload->result.stage_size, 16);
				}

				ArenaRelease(&upload->temp_arena);
			}
			
			G_SubmitImEnd(&cmd);
		}

		if (!G_ResourceKeyIsNull(staging_buffer))
			G_BufferDestroy(staging_buffer);

		base += batch_count;
	}
}

internal void A_DoAssetStreaming(void)
{
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);

	for (u32 i = 0; i < DensePoolLiveCount(&a_assets->record_pool); i++)
	{
		A_Record *record = &a_assets->records[i];

		A_LoaderAPI *api = &a_assets->loaders[record->type].api;

		if (!api->is_streamable)
			continue;

		u32 cmp = A_ResidencyLevelCompare(record->target_residency, record->current_residency);

		if (cmp > 0)
		{
			//record->load_state = A_LoadState_Loading;
			
			//record->is_streaming = true;

			DebugLogD(a_assets->log_channel,
					  "(not implemented) Streaming in/upgrading: %.*s",
					  String8VArg(record->metadata.path));
		}
		else if (cmp < 0)
		{
			DebugLogD(a_assets->log_channel,
					  "(not implemented) Streaming out/evicting: %.*s",
					  String8VArg(record->metadata.path));

			record->current_residency = record->target_residency;
		}
	}
	
	osapi->SpinLockRelease(&a_assets->registry_spinlock);
}

internal void A_SetStreamingState(A_Handle handle, const A_LoadResidencyCtx *residency_ctx)
{
	A_Record *record = A_GetRecord(handle);

	if (!record)
		return;

	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
	
	A_LoaderAPI *api = &a_assets->loaders[record->type].api;

	record->target_residency = api->CalculateResidency(residency_ctx);

	// todo: calculate a priority as well?
	// maybe something like:
	//   record->streaming_priority = A_CalcStreamingPriority(residency_ctx, frames_since_last_used);

	osapi->SpinLockRelease(&a_assets->registry_spinlock);
}

internal A_Type A_GetAssetTypeFromPath(String8 path)
{
	u64 last = String8FindLastIncl(path, String8Lit("."));
	String8 extension = String8Substr(path, last, path.len);

	for (u32 i = A_Type_Null + 1; i < A_Type_COUNT; i++)
	{
		A_Type type = i;
		
		A_LoaderAPI *api = &a_assets->loaders[type].api;

		for (u32 j = 0; j < api->file_extension_count; j++)
		{
			if (String8Match(extension, api->file_extensions[j]))
				return type;
		}
	}

	DebugLogB(a_assets->log_channel, "Couldn't find any asset loader for extension: \".%.*s\"", String8VArg(extension));
	
	return A_Type_Null;
}

internal A_Handle A_HandleFromFilePath(String8 path)
{
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
	
	A_Handle existing = A_PathMapFind(path);

	if (!A_HandleIsNull(existing))
	{
		osapi->SpinLockRelease(&a_assets->registry_spinlock);
		return existing;
	}
	else
	{
		ScratchArena scratch = ScratchBegin(NULL, 0);

		A_Type type = A_GetAssetTypeFromPath(path);
		
		A_Record *record = A_AllocRecord(type);

		String8 sys_path = A_GetSystemFilePath(scratch.arena, path);

		record->metadata.path = String8Clone(a_assets->arena, path);
		record->metadata.last_write_time = osapi->GetFileLastWriteTime(sys_path);
		
		A_Handle handle = {0};
		handle.id = record->id;
		handle.generation = record->generation;
		handle.type = type;

		A_PathMapInsert(path, handle);

		ScratchRelease(&scratch);
	
		osapi->SpinLockRelease(&a_assets->registry_spinlock);
		
		return handle;
	}
}

internal A_Asset *A_GetOrFallback(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	if (record && record->load_state == A_LoadState_Ready)
		return &record->asset;

	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 type_string = A_StringFromType(scratch.arena, handle.type);

	if (record)
	{
		DebugLogW(a_assets->log_channel,
				  "%.*s asset not found (path: %.*s). Falling back...",
				  String8VArg(type_string),
				  String8VArg(record->metadata.path));	
	}
	else
	{
		DebugLogW(a_assets->log_channel,
				  "%.*s asset not found. Falling back...",
				  String8VArg(type_string));
	}
	
	A_Record *fallback = A_GetRecord(a_assets->loaders[handle.type].fallback);

	if (!fallback)
	{
		DebugLogB(a_assets->log_channel,
				  "No fallback found. We're fucked.",
				  String8VArg(type_string));
		
		ScratchRelease(&scratch);

		return NULL;
	}
	
	ScratchRelease(&scratch);

	return &fallback->asset;
}

internal A_Asset *A_GetOrBreak(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	if (!record)
	{
		DebugLogB(a_assets->log_channel, "Asset not found.");
	}
	else if (record->load_state != A_LoadState_Ready)
	{
		DebugLogB(a_assets->log_channel, "Asset not ready.");
	}
	else
	{
		return &record->asset;
	}
	
	return NULL;
}

internal J_ENTRY_POINT_DEF(A_LoadJob)
{
	A_LoadJobParam *load_params = param;

	A_Type type = load_params->record->type;
	
	Arena job_arena = ArenaAlloc(A_JOB_ARENA_ALLOC_SIZE);

	A_LCTX ctx = {0};
	ctx.log_channel = a_assets->loaders[type].log_channel;
	ctx.metadata = load_params->record->metadata;
	ctx.current_residency = load_params->record->current_residency;
	ctx.target_residency = load_params->record->target_residency;
	
	A_LoaderAPI *api = &a_assets->loaders[type].api;

	A_LoadResult result = api->Load(&ctx, &job_arena);

	if (result.failed)
	{
		DebugLogD(a_assets->log_channel,
				  "Failed to load: %.*s",
				  String8VArg(ctx.metadata.path));
	}
	else
	{
		DebugLogD(a_assets->log_channel,
				  "Loaded: %.*s",
				  String8VArg(ctx.metadata.path));

		ScratchArena scratch = ScratchBegin(NULL, 0);

		J_Decl *decls = ArenaPushArray(scratch.arena, J_Decl, result.dependency_count);
		A_LoadJobParam *params = ArenaPushArray(scratch.arena, A_LoadJobParam, result.dependency_count);
		
		for (u32 i = 0; i < result.dependency_count; i++)
		{
			A_Record *record = A_GetRecord(result.dependencies[i]);

			osapi->SpinLockAcquire(&a_assets->registry_spinlock);
			
			if (record->load_state == A_LoadState_Unloaded)
			{
				record->load_state = A_LoadState_Loading;
	
				params[i].record = record;
				params[i].counter = load_params->counter;
	
				decls[i].EntryPoint = A_LoadJob;
				decls[i].param = &params[i];
				decls[i].priority = J_Priority_Normal;
				decls[i].flags = J_Flag_None;
			}
			
			osapi->SpinLockRelease(&a_assets->registry_spinlock);
		}

		osapi->JobBatch(decls, result.dependency_count, load_params->counter);
		
		ScratchRelease(&scratch);
	}

	A_Upload upload = {0};
	upload.record = load_params->record;
	upload.temp_arena = job_arena;
	upload.result = result;

	osapi->SpinLockAcquire(&a_assets->upload_spinlock);
	a_assets->upload_queue[a_assets->upload_count++] = upload;
	osapi->SpinLockRelease(&a_assets->upload_spinlock);
}

internal A_Handle A_RequireAsset(String8 path, OS_Handle counter)
{
	A_Handle handle = A_HandleFromFilePath(path);
	A_Record *record = A_GetRecord(handle);
	
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
	
	if (record->load_state != A_LoadState_Unloaded)
	{
		osapi->SpinLockRelease(&a_assets->registry_spinlock);
		return handle;
	}
	else
	{
		record->load_state = A_LoadState_Loading;
		osapi->SpinLockRelease(&a_assets->registry_spinlock);
	}

	// TODO: Allocating the params onto the system arena right now. Problem is that
	//       it's gonna be wasted memory after loading but it's so minor I don't
	//       think it matters at all.
	A_LoadJobParam *param = ArenaPushArray(a_assets->arena, A_LoadJobParam, 1);
	param->record = record;
	param->counter = counter;
	
	J_Decl decl = {0};
	decl.EntryPoint = A_LoadJob;
	decl.param = param;
	decl.priority = J_Priority_Normal;
	decl.flags = J_Flag_None;

	osapi->JobKick(&decl, counter);

	return handle;
}

internal A_Handle A_RequireAssetBlocking(String8 path)
{
	OS_Handle counter = osapi->JobCounterAlloc(0);
	A_Handle handle = A_RequireAsset(path, counter);
	A_WaitForLoadAndRelease(counter);
	return handle;
}

internal void A_DestroyAsset(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);
	
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
		
	A_LoaderAPI *api = &a_assets->loaders[record->type].api;

	if (api->DestroyAsset)
		api->DestroyAsset(&record->asset);

	A_FreeRecord(record);
	
	A_PathMapRemove(record->metadata.path);
	
	osapi->SpinLockRelease(&a_assets->registry_spinlock);
}

internal void A_WaitForLoad(OS_Handle counter)
{
	while (osapi->JobCounterValue(counter) > 0)
		A_FlushUploads();

	A_FlushUploads();
}

internal void A_WaitForLoadAndRelease(OS_Handle counter)
{
	A_WaitForLoad(counter);
	osapi->JobCounterRelease(counter);
}

internal void A_Mount(String8 prefix, String8 directory)
{
	DebugLogAssert(a_assets->log_channel, directory.len > 0, "Directory length must be greater than zero.");
	DebugLogAssert(a_assets->log_channel, a_assets->mount_point_count < ArraySize(a_assets->mount_points), "Cannot mount more directories.");

	A_MountPoint *mp = &a_assets->mount_points[a_assets->mount_point_count++];
	
	mp->prefix = String8Clone(a_assets->arena, prefix);
	mp->directory = String8Clone(a_assets->arena, directory);
}

internal String8 A_GetSystemFilePath(Arena *arena, String8 path)
{
	A_MountPoint *best = NULL;
	u64 best_len = 0;

	for (u32 i = 0; i < a_assets->mount_point_count; i++)
	{
		A_MountPoint *m = &a_assets->mount_points[i];

		if (String8StartsWith(path, m->prefix) && m->prefix.len > best_len)
		{
			best = m;
			best_len = m->prefix.len;
		}
	}

	if (!best)
	{
		DebugLogB(a_assets->log_channel,
				  "Failed to find system file path for asset path: \"%.*s\"",
				  String8VArg(path));
	}
	
	String8 suffix = String8Skip(path, best_len);

	return IO_PathJoin(arena, best->directory, suffix);
}
