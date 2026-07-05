
static A_Assets *a_assets = NULL;

static u32 A_AllocSlot(void)
{
	if (a_assets->free_index_count > 0)
		return a_assets->free_indices[--a_assets->free_index_count];

	DebugLogAssert(a_assets->log_channel,
				   a_assets->record_count < ArraySize(a_assets->records),
				   "Cannot allocate more asset records, out of space!");

	return a_assets->record_count++;
}

static void A_FreeSlot(u32 index)
{
	DebugLogAssert(a_assets->log_channel,
				   a_assets->free_index_count < ArraySize(a_assets->free_indices),
				   "Cannot free more asset records, out of free index space!");

	a_assets->free_indices[a_assets->free_index_count] = index;
	a_assets->free_index_count++;
}

static A_Handle A_AllocRecord(String8 path)
{
	u32 index = A_AllocSlot();

	A_Record *record = &a_assets->records[index];

	u32 prev_gen = record->generation;

	MemZeroStruct(record);

	record->path = String8Clone(a_assets->arena, path);
	record->state = A_RecordState_Unloaded;
	
	record->generation = prev_gen + 1;

	A_Handle handle = {0};
	handle.index = index;
	handle.generation = record->generation;

	return handle;
}

static A_Record *A_GetRecord(A_Handle handle)
{
	if (!A_IsValid(handle))
		return NULL;
	
	return &a_assets->records[handle.index];
}

static const A_Record *A_GetRecordConst(A_Handle handle)
{
	if (!A_IsValid(handle))
		return NULL;

	return &a_assets->records[handle.index];
}

static A_Handle A_PathMapFind(String8 path)
{
	// shitty but cool trick but i really shouldnt be doing this lol
	// note: A_MANAGER_MAX_RECORDS *must* be a power of two for this to work.
	
	const u64 hash = HashStr8(path);
	const u32 mask = A_MANAGER_MAX_RECORDS - 1;

	for (u32 i = 0; i < A_MANAGER_MAX_RECORDS; i++)
	{
		u32 idx = (u32)((hash + i) & mask);

		const A_PathMapEntry *entry = &a_assets->path_map[idx];
		
		if (!entry->occupied)
			break;

		if (String8Match(entry->path, path))
			return entry->value;
	}

	return A_HandleNull();
}

static void A_PathMapInsert(String8 path, A_Handle handle)
{
	DebugLogAssert(a_assets->log_channel, A_IsValid(handle), "Asset handle invalid.");

	const u64 hash = HashStr8(path);
	const u32 mask = A_MANAGER_MAX_RECORDS - 1;

	for (u32 i = 0; i < A_MANAGER_MAX_RECORDS; i++)
	{
		u32 idx = (u32)((hash + i) & mask);

		A_PathMapEntry *entry = &a_assets->path_map[idx];
		
		if (!entry->occupied)
		{
			entry->occupied = true;
			
			entry->value = handle;
			entry->path = String8Clone(a_assets->arena, path);
			
			return;
		}
	}

	DebugLogB(a_assets->log_channel, "Cannot add more paths to path map.");
}

static u32 A_LoadArenaAcquire(void)
{
	for (;;)
	{
		osapi->SpinLockAcquire(&a_assets->load_arena_spinlock);

		if (a_assets->free_load_arena_count > 0)
		{
			a_assets->free_load_arena_count--;
			u32 index = a_assets->free_load_arenas[a_assets->free_load_arena_count];
			osapi->SpinLockRelease(&a_assets->load_arena_spinlock);
			return index;
		}

		osapi->JobCounterInc(a_assets->load_arena_wait_counter, 1);
		osapi->SpinLockRelease(&a_assets->load_arena_spinlock);

		osapi->JobYield(a_assets->load_arena_wait_counter, 0);
	}
}

static void A_LoadArenaRelease(u32 index)
{
	ArenaResetAndDecommit(&a_assets->load_arenas[index]);
 
	osapi->SpinLockAcquire(&a_assets->load_arena_spinlock);
 
	a_assets->free_load_arenas[a_assets->free_load_arena_count] = index;
	a_assets->free_load_arena_count++;

	u32 waiters = osapi->JobCounterValue(a_assets->load_arena_wait_counter);

	osapi->SpinLockRelease(&a_assets->load_arena_spinlock);

	if (waiters > 0)
		osapi->JobCounterDec(a_assets->load_arena_wait_counter, 1);
}

static void A_InitAndSelect(A_Assets *assets, Arena *arena, LOG_Channel log_channel)
{
	assets->arena = arena;

	assets->log_channel = log_channel;

	assets->load_arena_wait_counter = osapi->JobCounterAlloc(0);
	assets->async_counter = osapi->JobCounterAlloc(0);

	assets->upload_mutex = osapi->MutexCreate();
	assets->dependency_mutex = osapi->MutexCreate();
	assets->allocation_mutex = osapi->MutexCreate();
	assets->loading_mutex = osapi->MutexCreate();
	assets->loading_cond = osapi->CondVarCreate();

	for (u32 i = 0; i < A_LOAD_ARENA_COUNT; i++)
	{
		assets->load_arenas[i] = ArenaAlloc(A_LOAD_ARENA_RESERVE);
		assets->free_load_arenas[i] = i;
	}

	assets->free_load_arena_count = A_LOAD_ARENA_COUNT;

#define AssetDef(name, upper)											\
	assets->serializers[A_Type_##name] = A_Get##name##Serializer(); \
	assets->serializer_log_channels[A_Type_##name] = osapi->LogChannelOpenFrom(log_channel, String8Lit(STRINGIFY(upper)));
#include "asset_definitions.inc"
#undef AssetDef

	A_SelectContext(assets);
	
	DebugLogI(assets->log_channel, "Initialized.");
}

static void A_Destroy(void)
{
	for (u32 i = 0; i < a_assets->record_count; i++)
	{
		A_Record *record = &a_assets->records[i];

		if (record->state == A_RecordState_Ready || record->reloading)
		{
			A_Serializer *s = &a_assets->serializers[record->asset.handle.type];

			if (s->Dispose)
				s->Dispose(&record->asset);
		}
	}

	for (u32 i = 0; i < A_LOAD_ARENA_COUNT; i++)
	{
		ArenaRelease(&a_assets->load_arenas[i]);
	}

	osapi->JobCounterRelease(a_assets->load_arena_wait_counter);
	osapi->JobCounterRelease(a_assets->async_counter);
	
	osapi->MutexDestroy(a_assets->upload_mutex);
	osapi->MutexDestroy(a_assets->dependency_mutex);
	osapi->MutexDestroy(a_assets->allocation_mutex);
	osapi->MutexDestroy(a_assets->loading_mutex);
	osapi->CondVarDestroy(a_assets->loading_cond);

	DebugLogI(a_assets->log_channel, "Destroyed.");

	a_assets = NULL;
}

static void A_SelectContext(A_Assets *assets)
{
	a_assets = assets;
}

static void A_Mount(String8 prefix, String8 directory)
{
	DebugLogAssert(a_assets->log_channel, directory.len > 0, "Directory length must be greater than zero.");
	DebugLogAssert(a_assets->log_channel, a_assets->mount_point_count < ArraySize(a_assets->mount_points), "Cannot mount more directories, out of space!");

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

static b32 A_IsLoaded(A_Handle handle)
{
	const A_Record *record = A_GetRecordConst(handle);

	if (!record)
		return false;

	A_RecordState st = record->state;
	
	return
		st == A_RecordState_Ready ||
		st == A_RecordState_Failed;
}

static b32 A_IsLoading(A_Handle handle)
{
	const A_Record *record = A_GetRecordConst(handle);

	if (!record)
		return false;

	A_RecordState st = record->state;

	return
		st == A_RecordState_CpuStage ||
		st == A_RecordState_WaitingForDependencies ||
		st == A_RecordState_GpuStage;
}

static b32 A_IsValid(A_Handle handle)
{
	return (handle.index < a_assets->record_count &&
			a_assets->records[handle.index].generation == handle.generation);
}

static void A_LoadNow(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	OS_Handle counter = osapi->JobCounterAlloc(0);
		
	if (record->state == A_RecordState_Unloaded)
	{
		A_Load(handle, counter);
		osapi->JobYield(counter, 0);
	}

	A_WaitForLoad(handle, counter);
	
	osapi->JobCounterRelease(counter);
}

static void A_LoadAsync(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	if (record->state == A_RecordState_Unloaded)
		A_Load(handle, a_assets->async_counter);
}

static void A_ReloadAsync(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	if (record->state != A_RecordState_Unloaded)
		A_Load(handle, a_assets->async_counter);
}

static J_ENTRY_POINT_DEF(A_LoadJobEntry)
{
	A_LoadJobParam *load_params = param;

	u32 arena_index = A_LoadArenaAcquire();
	Arena *load_arena = &a_assets->load_arenas[arena_index];
	
	A_Context ctx = {0};
	ctx.metadata = load_params->metadata;
	ctx.log_channel = a_assets->serializer_log_channels[load_params->handle.type];

	A_Serializer *serializer = &a_assets->serializers[load_params->handle.type];

	A_SerializerPipelineData load_data = serializer->Cpu(&ctx, load_arena);

	if (load_data.failed)
	{
		DebugLogE(a_assets->log_channel,
				  "Failed to load %.*s.",
				  String8VArg(ctx.metadata.path));
	}
	else
	{
		DebugLogD(a_assets->log_channel,
				  "Loaded in %.*s.",
				  String8VArg(ctx.metadata.path));
	}

	A_Upload upload = {0};
	upload.load_arena_index = arena_index;
	upload.metadata = load_params->metadata;
	upload.handle = load_params->handle;
	upload.load_data = load_data;
 
	osapi->MutexLock(a_assets->dependency_mutex);
	A_UploadQueuePush(&a_assets->dependency_queue, &upload);
	osapi->MutexUnlock(a_assets->dependency_mutex);
}

static void A_Load(A_Handle handle, OS_Handle counter)
{
	A_Record *r = &a_assets->records[handle.index];
	
	if (r->state == A_RecordState_Ready)
		r->reloading = true;
	
	r->state = A_RecordState_CpuStage;

	A_MetaData metadata = {0};
	metadata.path = A_GetRecord(handle)->path;

	// params get dumped onto the permanent arena which isnt that big a deal
	// 'cuz they're only like a couple of bytes so whatever.
	// TODO - fix this leak somehow :p
	osapi->MutexLock(a_assets->allocation_mutex);
	A_LoadJobParam *params = ArenaPushArray(a_assets->arena, A_LoadJobParam, 1);
	osapi->MutexUnlock(a_assets->allocation_mutex);
	
	params->metadata = metadata;
	params->handle = handle;
 
	J_Decl decl = {0};
	decl.EntryPoint = A_LoadJobEntry;
	decl.priority = J_Priority_Normal;
	decl.param = params;
	
	osapi->JobKick(&decl, counter);
}

static void A_NotifyDependents(A_Handle handle)
{
	osapi->MutexLock(a_assets->dependency_mutex);
	A_NotifyDependentsNoLock(handle, false);
	osapi->MutexUnlock(a_assets->dependency_mutex);
}

static void A_NotifyDependentsNoLock(A_Handle handle, b32 failed)
{
	DebugLogAssert(a_assets->log_channel, A_IsValid(handle), "Asset handle must be valid.");

	A_Record *record = A_GetRecord(handle);

	for (u32 i = 0; i < record->dependent_count; i++)
	{
		A_Handle  parent_handle = record->dependents[i];
		A_Record *parent_record = A_GetRecord(parent_handle);

		if (failed)
		{
			if (parent_record->reloading)
			{
				parent_record->state = A_RecordState_Ready;
				parent_record->reloading = false;
			}
			else
			{
				parent_record->state = A_RecordState_Failed;
				A_NotifyDependentsNoLock(parent_handle, true);
			}
		}
		else
		{
			if (parent_record->hot_pending_dependencies > 0)
				parent_record->hot_pending_dependencies--;

			if (parent_record->hot_pending_dependencies == 0 &&
				parent_record->state == A_RecordState_WaitingForDependencies)
			{
				parent_record->state = A_RecordState_GpuStage;

				osapi->MutexLock(a_assets->upload_mutex);
				A_UploadQueuePush(&a_assets->upload_queue, &parent_record->stashed_upload);
				osapi->MutexUnlock(a_assets->upload_mutex);
			}
		}
	}

	record->dependent_count = 0;
}

static void A_ResolvePendingDependencies(OS_Handle counter)
{
	osapi->MutexLock(a_assets->dependency_mutex);

	A_UploadQueue *q = &a_assets->dependency_queue;

	for (u32 i = 0; i < q->count; i++)
	{
		A_Upload *upload = &q->elements[i];
		A_Record *record = A_GetRecord(upload->handle);

		if (upload->load_data.failed)
		{
			b32 was_reloading = record->reloading;
			
			if (was_reloading)
			{
				DebugLogW(a_assets->log_channel,
						  "Reload failed for %.*s, keeping previous version.",
						  String8VArg(upload->metadata.path));
				
				record->state = A_RecordState_Ready;
				record->reloading = false;
			}
			else
			{
				record->state = A_RecordState_Failed;
			}

			A_NotifyDependentsNoLock(upload->handle, !was_reloading);

			A_LoadArenaRelease(upload->load_arena_index);

			continue;
		}

		u32 unresolved = 0;

		for (u32 j = 0; j < upload->load_data.dependency_count; j++)
		{
			A_Handle  dep_handle = upload->load_data.dependencies[j];
			A_Record *dep_record = A_GetRecord(dep_handle);

			if (dep_record->state == A_RecordState_Unloaded)
			{
				A_Load(dep_handle, counter);
			}
			
			if (dep_record->state != A_RecordState_Ready &&
				dep_record->state != A_RecordState_Failed)
			{
				unresolved++;

				DebugLogAssert(a_assets->log_channel,
							   dep_record->dependent_count < ArraySize(dep_record->dependents),
							   "Ran out of space in dependents array of asset record.");

				dep_record->dependents[dep_record->dependent_count] = upload->handle;
				dep_record->dependent_count++;
			}
		}

		if (unresolved > 0)
		{
			record->state = A_RecordState_WaitingForDependencies;
			record->hot_pending_dependencies = unresolved;
			record->stashed_upload = *upload;
		}
		else
		{
			record->state = A_RecordState_GpuStage;

			osapi->MutexLock(a_assets->upload_mutex);
			A_UploadQueuePush(&a_assets->upload_queue, upload);
			osapi->MutexUnlock(a_assets->upload_mutex);
		}
	}

	A_UploadQueueClear(q);

	osapi->MutexUnlock(a_assets->dependency_mutex);
}

static void A_PollHotReloads(void)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
 
	for (u32 i = 0; i < a_assets->record_count; i++)
	{
		A_Record *record = &a_assets->records[i];
 
		if (record->state != A_RecordState_Ready)
			continue;
 
		String8 sys_path = A_GetSystemFilePath(scratch.arena, record->path);
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
 
			A_Handle handle = record->asset.handle;
 
			A_ReloadAsync(handle);
		}
 
		ScratchClear(&scratch);
	}
 
	ScratchRelease(&scratch);
}

static void A_FlushUploads(void)
{
	A_ResolvePendingDependencies(a_assets->async_counter);

	osapi->MutexLock(a_assets->upload_mutex);

	if (a_assets->upload_queue.count == 0)
	{
		osapi->MutexUnlock(a_assets->upload_mutex);
		return;
	}

	A_UploadQueue pending = a_assets->upload_queue;
	A_UploadQueueClear(&a_assets->upload_queue);

	osapi->MutexUnlock(a_assets->upload_mutex);

	u32 base = 0;

	while (base < pending.count)
	{
		u64 batch_stage_size = 0;
		u64 batch_count = 0;

		for (u32 i = base; i < pending.count; i++)
		{
			u64 upload_size  = pending.elements[i].load_data.stage_size;
			u64 aligned_size = MemAlignUp(upload_size, 16);

			if (aligned_size > A_GPU_UPLOAD_CHUNK && batch_stage_size == 0)
			{
				batch_stage_size = aligned_size;
				batch_count = 1;
				
				break;
			}

			if (batch_stage_size + aligned_size > A_GPU_UPLOAD_CHUNK)
				break;

			batch_stage_size += aligned_size;
			batch_count++;
		}

		G_BufferKey staging_buffer = G_BufferKeyNull();

		if (batch_stage_size > 0)
			staging_buffer = G_DeviceStageAlloc(batch_stage_size);

		G_CmdBuffer cmd = G_DeviceSubmitImBegin();
		{
			u64 stage_offset = 0;

			for (u32 i = 0; i < batch_count; i++)
			{
				A_Upload *upload = &pending.elements[base + i];
				A_Record *record = A_GetRecord(upload->handle);

				A_Serializer *serializer = &a_assets->serializers[upload->handle.type];

				A_Asset *asset = &record->asset;

				A_Context ctx = {0};
				ctx.metadata = upload->metadata;
				ctx.log_channel = a_assets->serializer_log_channels[upload->handle.type];

				if (upload->load_data.failed)
				{
					if (record->reloading)
					{
						DebugLogW(a_assets->log_channel,
								  "Reload failed for %.*s, keeping previous version.",
								  String8VArg(upload->metadata.path));
						
						record->state = A_RecordState_Ready;
						record->reloading = false;
					}
					else
					{
						record->state = A_RecordState_Failed;
					}
				}
				else
				{
					b32 is_new = asset->handle.type == A_Type_Null;

					asset->handle = upload->handle;

					if (is_new)
					{
						osapi->MutexLock(a_assets->allocation_mutex);
						serializer->Alloc(&ctx, &upload->load_data, asset, a_assets->arena);
						osapi->MutexUnlock(a_assets->allocation_mutex);
					}
					else
					{
						serializer->Reload(&ctx, &upload->load_data, asset);
					}
					
					if (serializer->Gpu)
						serializer->Gpu(&ctx, &upload->load_data, asset, &cmd, staging_buffer, stage_offset);

					if (serializer->End)
						serializer->End(&upload->load_data);

					if (upload->load_data.watch_path_count > 0)
					{
						osapi->MutexLock(a_assets->allocation_mutex);

						record->watch_path_count = upload->load_data.watch_path_count;
						record->watch_paths = ArenaPushArray(a_assets->arena, String8, record->watch_path_count);

						for (u32 j = 0; j < record->watch_path_count; j++)
							record->watch_paths[j] = String8Clone(a_assets->arena, upload->load_data.watch_paths[j]);

						osapi->MutexUnlock(a_assets->allocation_mutex);
					}

					DebugLogD(a_assets->log_channel,
							  "%s %.*s.",
							  is_new ? "Allocated" : "Reloaded",
							  String8VArg(upload->metadata.path));

					ScratchArena scratch = ScratchBegin(NULL, 0);
					{
						String8 sys_path = A_GetSystemFilePath(scratch.arena, record->path);
						record->last_write_time = osapi->GetFileLastWriteTime(sys_path);
 
						for (u32 j = 0; j < record->watch_path_count; j++)
						{
							u64 t = osapi->GetFileLastWriteTime(record->watch_paths[j]);
 
							if (t > record->last_write_time)
								record->last_write_time = t;
						}
					}
					ScratchRelease(&scratch);

					record->state = A_RecordState_Ready;
					record->reloading = false;

					A_NotifyDependents(upload->handle);

					stage_offset += MemAlignUp(upload->load_data.stage_size, 16);
				}
				
				A_LoadArenaRelease(upload->load_arena_index);
			}
		}
		G_DeviceSubmitImEnd(&cmd);

		if (!G_BufferKeyIsNull(staging_buffer))
			G_DeviceBufferDestroy(staging_buffer);

		base += batch_count;
	}

	osapi->CondVarBroadcast(a_assets->loading_cond);
}

static void A_WaitForAsync(void)
{
	osapi->JobYield(a_assets->async_counter, 0);
}

static void A_WaitForLoad(A_Handle handle, OS_Handle counter)
{
	A_Record *record = A_GetRecord(handle);

	if (!record)
		return;

	while (record->state != A_RecordState_Ready &&
		   record->state != A_RecordState_Failed)
	{
		A_ResolvePendingDependencies(counter);
		osapi->JobYield(counter, 0);
		A_FlushUploads();
	}
}

static void A_SetFallback(A_Handle handle, A_Type type)
{
	DebugLogAssert(a_assets->log_channel, A_IsLoaded(handle), "Fallback asset hasn't loaded yet.");

	a_assets->fallbacks[type] = handle;
}

static A_Asset *A_Get(A_Handle handle)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	A_Asset *selected = NULL;
	
	A_Record *record = A_GetRecord(handle);

	A_Type type = handle.type;
	
	if (record &&
		(record->state == A_RecordState_Ready || record->reloading) &&
		record->asset.handle.type == type)
	{
		selected = &record->asset;
		goto end;
	}

	String8 type_string = A_StringFromType(scratch.arena, type);

	DebugLogW(a_assets->log_channel,
			  "%.*s asset not found. Falling back...",
			  String8VArg(type_string));
	
	A_Record *fallback = A_GetRecord(a_assets->fallbacks[type]);

	if (fallback && fallback->state == A_RecordState_Ready)
	{
		selected = &fallback->asset;
		goto end;
	}

	DebugLogB(a_assets->log_channel,
			  "No fallback found for %.*s asset. We're fucked basically.",
			  String8VArg(type_string));

	selected = &a_assets->null_asset_sentinel;

end:
	ScratchRelease(&scratch);
	return selected;
}

static A_Asset *A_GetNow(A_Handle handle)
{
	A_Record *record = A_GetRecord(handle);

	if (!record)
		return NULL;

	if (record->state == A_RecordState_Unloaded)
	{
		A_LoadNow(handle);
	}
	else if (A_IsLoading(handle))
	{
		A_WaitForLoad(handle, a_assets->async_counter);
		/*
		OS_Handle counter = osapi->JobCounterAlloc(0);
		A_WaitForLoad(handle, counter);
		osapi->JobCounterRelease(counter);
		*/
	}

	return A_Get(handle);
}

static A_Handle A_FromFilePath(String8 path, A_Type type)
{
	osapi->MutexLock(a_assets->allocation_mutex);

	A_Handle existing = A_PathMapFind(path);

	if (A_IsValid(existing))
	{
		osapi->MutexUnlock(a_assets->allocation_mutex);		
		return existing;
	}

	A_Handle handle = A_AllocRecord(path);
	handle.type = type;

	A_PathMapInsert(path, handle);

	osapi->MutexUnlock(a_assets->allocation_mutex);

	return handle;
}

static A_Handle A_Require(String8 path, A_Type type)
{
	A_Handle handle = A_FromFilePath(path, type);

	A_LoadAsync(handle);

	return handle;
}
