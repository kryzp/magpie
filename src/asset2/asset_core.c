
static A_State *a_assets = NULL;

static A_Handle A_PathMapFind(String8 path)
{
	for (A_PathMapEntry *entry = a_assets->first_path_entry; entry; entry = entry->next)
	{
		if (String8Match(entry->path, path))
			return entry->handle;
	}
	
	return A_HandleNull();
}

static void A_PathMapInsert(String8 path, A_Handle handle)
{
	A_PathMapEntry *entry = ArenaPushArray(a_assets->arena, A_PathMapEntry, 1);
	entry->next = a_assets->first_path_entry;
	a_assets->first_path_entry = entry;

	entry->path = path;
	entry->handle = handle;
}

static A_Record *A_AllocRecord(A_Type type)
{
	A_Record *record = NULL;

	if (a_assets->free_record_sentinel.next != &a_assets->free_record_sentinel)
	{
		record = a_assets->free_record_sentinel.next;

		record->prev->next = record->next;
		record->next->prev = record->prev;

		MemZeroStruct(record);
	}
	else
	{
		record = ArenaPushArray(a_assets->arena, A_Record, 1);
	}

	record->uid = a_assets->current_record_uid;
	a_assets->current_record_uid++;

	record->next = a_assets->record_sentinel.next;
	record->prev = &a_assets->record_sentinel;

	record->next->prev = record;
	record->prev->next = record;

	record->uid = a_assets->current_record_uid++;
	record->type = type;
	
	return record;
}

static void A_FreeRecord(A_Record *record)
{
	record->prev->next = record->next;
	record->next->prev = record->prev;

	record->next = a_assets->free_record_sentinel.next;
	record->prev = &a_assets->free_record_sentinel;

	record->next->prev = record;
	record->prev->next = record;
}

static A_Record *A_GetRecord(A_Handle handle)
{
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);

	A_Record *found = NULL;
	
	for (A_Record *record = a_assets->record_sentinel.next;
		 record != &a_assets->record_sentinel;
		 record = record->next)
	{
		if (record->uid == handle.uid)
		{
			found = record;
			break;
		}
	}

	osapi->SpinLockRelease(&a_assets->registry_spinlock);
	
	return found;
}

static void A_InitAndSelect(A_State *state, Arena *arena, LOG_Channel log_channel)
{
	state->arena = arena;
	state->log_channel = log_channel;
	
	state->record_sentinel.next = &state->record_sentinel;
	state->record_sentinel.prev = &state->record_sentinel;

	state->free_record_sentinel.next = &state->free_record_sentinel;
	state->free_record_sentinel.prev = &state->free_record_sentinel;

	state->current_record_uid = 1; // start at 1 as current_record_uid=0=null
	
#define AssetDef(name, upper)											\
	state->loaders[A_Type_##name].api = A_Get##name##LoaderAPI();		\
	state->loaders[A_Type_##name].log_channel = osapi->LogChannelOpenFrom(log_channel, String8Lit(STRINGIFY(upper))); \
	state->loaders[A_Type_##name].fallback = A_HandleNull();
#include "asset_xmacro.inc"
#undef AssetDef

	A_SelectContext(state);
	
	DebugLogI(state->log_channel, "Initialized.");
}

static void A_Destroy(void)
{
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
	
	for (A_Record *record = a_assets->record_sentinel.next;
		 record != &a_assets->record_sentinel;
		 record = record->next)
	{
		A_LoaderAPI *api = &a_assets->loaders[record->type].api;

		if (api->DestroyAsset)
			api->DestroyAsset(&record->asset);
	}

	osapi->SpinLockRelease(&a_assets->registry_spinlock);

	DebugLogI(a_assets->log_channel, "Destroyed.");

	a_assets = NULL;
}

static void A_SelectContext(A_State *state)
{
	a_assets = state;
}

static void A_SetFallback(A_Handle handle)
{
	a_assets->loaders[handle.type].fallback = handle;
}

static void A_PollHotReloads(void)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
	
	for (A_Record *record = a_assets->record_sentinel.next;
		 record != &a_assets->record_sentinel;
		 record = record->next)
	{
		if (record->load_state != A_LoadState_Ready)
			continue;

		String8 system_path = A_GetSystemFilePath(scratch.arena, record->metadata.path);

		u64 latest_write = osapi->GetFileLastWriteTime(system_path);

		if (latest_write > record->metadata.last_write_time && latest_write > 0)
		{
			record->metadata.last_write_time = latest_write;

			// reload??? ?!?? ? ?! ?
			// how???
			// need to somehow reload the data inside of the arena in which the asset
			// is stored, but it's stored in a group alloc with a bunch of others.... :(
			// shoot.

			DebugLogE(a_assets->log_channel, "(not implemented, should be reloading: %.*s)", String8VArg(record->metadata.path));
		}
		
		ScratchClear(&scratch);
	}

	osapi->SpinLockRelease(&a_assets->registry_spinlock);
	
	ScratchRelease(&scratch);
}

static void A_FlushUploads(void)
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

		G_BufferKey staging_buffer = G_BufferKeyNull();

		if (batch_stage_size > 0)
			staging_buffer = G_DeviceStageAlloc(batch_stage_size);

		{
			G_CmdBuffer cmd = G_DeviceSubmitImBegin();
		
			u64 stage_offset = 0;

			for (u32 i = 0; i < batch_count; i++)
			{
				A_Upload *upload = &pending[base + i];
				A_Record *record = upload->record;

				A_TypeLoader *loader = &a_assets->loaders[record->type];

				A_LCTX ctx = {0};
				ctx.log_channel = loader->log_channel;
				ctx.metadata = record->metadata;

				if (upload->result.failed)
				{
					/*
					if (record->reloading)
					{
						DebugLogW(a_assets->log_channel,
								  "Reload failed for %.*s, keeping previous version.",
								  String8VArg(upload->metadata.path));
						
						record->load_state = A_LoadState_Ready;
					}
					else
					{
						record->load_state = A_LoadState_Failed;
					}
					*/
					
					record->load_state = A_LoadState_Failed;
				}
				else
				{
					if (loader->api.Alloc)
						loader->api.Alloc(&ctx, &upload->result, upload->perm_arena, &record->asset);
					
					if (loader->api.UploadGPU)
						loader->api.UploadGPU(&ctx, &upload->result, &record->asset, &cmd, staging_buffer, stage_offset);

					if (loader->api.DestroyIntermediateResources)
						loader->api.DestroyIntermediateResources(&upload->result);

					DebugLogD(a_assets->log_channel,
							  "Finalized: %.*s",
							  String8VArg(record->metadata.path));
					
					record->load_state = A_LoadState_Ready;

					stage_offset += MemAlignUp(upload->result.stage_size, 16);
				}

				ArenaRelease(&upload->temp_arena);
			}
			
			G_DeviceSubmitImEnd(&cmd);
		}

		if (!G_BufferKeyIsNull(staging_buffer))
			G_DeviceBufferDestroy(staging_buffer);

		base += batch_count;
	}
}

static A_Handle A_HandleFromFilePath(String8 path, A_Type type)
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

		A_Record *record = A_AllocRecord(type);

		String8 sys_path = A_GetSystemFilePath(scratch.arena, path);

		record->metadata.path = String8Clone(a_assets->arena, path);
		record->metadata.last_write_time = osapi->GetFileLastWriteTime(sys_path);
		
		A_Handle handle = {0};
		handle.uid = record->uid;
		handle.type = type;

		A_PathMapInsert(path, handle);

		ScratchRelease(&scratch);
	
		osapi->SpinLockRelease(&a_assets->registry_spinlock);
		
		return handle;
	}
}

static A_Asset *A_GetOrFallback(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	if (record && record->load_state == A_LoadState_Ready)
		return &record->asset;

	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 type_string = A_StringFromType(scratch.arena, handle.type);

	DebugLogW(a_assets->log_channel,
			  "%.*s asset not found. Falling back...",
			  String8VArg(type_string));
	
	A_Record *fallback = A_GetRecord(a_assets->loaders[handle.type].fallback);

	if (!fallback)
	{
		DebugLogB(a_assets->log_channel,
				  "No fallback found. We're fucked.",
				  String8VArg(type_string));
	}
	
	ScratchRelease(&scratch);

	return &fallback->asset;
}

static A_Asset *A_GetOrBreak(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	if (record->load_state == A_LoadState_Ready)
		return &record->asset;

	DebugLogB(a_assets->log_channel, "Asset not found / ready.");

	return NULL;
}

static J_ENTRY_POINT_DEF(A_LoadJob)
{
	A_LoadJobParam *load_params = param;

	A_Type type = load_params->record->type;
	
	Arena job_arena = ArenaAlloc(A_JOB_ARENA_ALLOC_SIZE);

	A_LCTX ctx = {0};
	ctx.log_channel = a_assets->loaders[type].log_channel;
	ctx.metadata = load_params->record->metadata;
	
	A_LoaderAPI *api = &a_assets->loaders[type].api;

	A_LoadResult result = api->Load(&ctx, &job_arena);

	if (result.failed)
	{
		DebugLogE(a_assets->log_channel,
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
	
				params[i].arena = load_params->arena;
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
	upload.perm_arena = load_params->arena;
	upload.result = result;

	osapi->SpinLockAcquire(&a_assets->upload_spinlock);
	a_assets->upload_queue[a_assets->upload_count++] = upload;
	osapi->SpinLockRelease(&a_assets->upload_spinlock);
}

/*
 * TODO: Allocating the params onto the arena mutex right now. Problem is that
 *       it's gonna be wasted memory after loading but it's so minor I don't
 *       think it matters at all.
 */
static A_Handle A_RequireAsset(Arena *arena, String8 path, A_Type type, OS_Handle counter)
{
	A_Handle handle = A_HandleFromFilePath(path, type);
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

	A_LoadJobParam *param = ArenaPushArray(arena, A_LoadJobParam, 1);
	param->arena = arena;
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

static A_Handle A_RequireAssetBlocking(Arena *arena, String8 path, A_Type type)
{
	OS_Handle counter = osapi->JobCounterAlloc(0);
	A_Handle handle = A_RequireAsset(arena, path, type, counter);
	A_WaitForLoadAndRelease(counter);
	return handle;
}

static void A_DestroyAsset(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);
	
	osapi->SpinLockAcquire(&a_assets->registry_spinlock);
		
	A_LoaderAPI *api = &a_assets->loaders[record->type].api;

	if (api->DestroyAsset)
		api->DestroyAsset(&record->asset);

	A_FreeRecord(record);
	
	osapi->SpinLockRelease(&a_assets->registry_spinlock);
}

static void A_WaitForLoad(OS_Handle counter)
{
	while (osapi->JobCounterValue(counter) > 0)
		A_FlushUploads();

	A_FlushUploads();
}

static void A_WaitForLoadAndRelease(OS_Handle counter)
{
	A_WaitForLoad(counter);
	osapi->JobCounterRelease(counter);
}

static void A_Mount(String8 prefix, String8 directory)
{
	DebugLogAssert(a_assets->log_channel, directory.len > 0, "Directory length must be greater than zero.");
	DebugLogAssert(a_assets->log_channel, a_assets->mount_point_count < ArraySize(a_assets->mount_points), "Cannot mount more directories.");

	A_MountPoint *mp = &a_assets->mount_points[a_assets->mount_point_count++];
	
	mp->prefix = String8Clone(a_assets->arena, prefix);
	mp->directory = String8Clone(a_assets->arena, directory);
}

static String8 A_GetSystemFilePath(Arena *arena, String8 path)
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
