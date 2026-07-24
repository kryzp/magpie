#ifndef ASSET_CORE_H
#define ASSET_CORE_H

#define A_MAX_MOUNT_POINTS              8
#define A_UPLOAD_QUEUE_MAX_SIZE         512
#define A_GPU_UPLOAD_CHUNK_MAX_SIZE     Megabytes(256)
#define A_JOB_ARENA_ALLOC_SIZE          Megabytes(512)

typedef enum A_LoadState
{
	A_LoadState_Unloaded,
	A_LoadState_Loading,
	A_LoadState_Ready,
	A_LoadState_Failed,
	A_LoadState_COUNT
}
A_LoadState;

typedef struct A_Record A_Record;
struct A_Record
{
	A_Record *next;
	A_Record *prev;
	
	u32 uid;
	A_Type type;
	
	A_Asset asset;
	A_LoadState load_state;
	A_Metadata metadata;
};

typedef struct A_PathMapEntry A_PathMapEntry;
struct A_PathMapEntry
{
	A_PathMapEntry *next;
	String8 path;
	A_Handle handle;
};

typedef struct A_MountPoint A_MountPoint;
struct A_MountPoint
{
	String8 prefix;
	String8 directory;
};

typedef struct A_TypeLoader A_TypeLoader;
struct A_TypeLoader
{
	A_LoaderAPI api;
	LOG_Channel log_channel;
	A_Handle fallback;
};

typedef struct A_Upload A_Upload;
struct A_Upload
{
	A_Record *record;
	Arena temp_arena;
	Arena *perm_arena;
	A_LoadResult result;
};

typedef struct A_State A_State;
struct A_State
{
	Arena *arena;
	LOG_Channel log_channel;

	A_Record record_sentinel;
	A_Record free_record_sentinel;
	u32 current_record_uid;

	i32 registry_spinlock;

	A_TypeLoader loaders[A_Type_COUNT];

	A_PathMapEntry *first_path_entry;

	A_MountPoint mount_points[A_MAX_MOUNT_POINTS];
	u32 mount_point_count;

	A_Upload upload_queue[A_UPLOAD_QUEUE_MAX_SIZE];
	u32 upload_count;
	i32 upload_spinlock;
};


/* ==================================================
   UTILS
   ================================================== */

internal A_Handle A_PathMapFind(String8 path);
internal void A_PathMapInsert(String8 path, A_Handle handle);

internal A_Record *A_AllocRecord(A_Type type);
internal void A_FreeRecord(A_Record *record);

internal A_Record *A_GetRecord(A_Handle handle);


/* ==================================================
   CORE
   ================================================== */

internal void A_InitAndSelect(A_State *state, Arena *arena, LOG_Channel log_channel);
internal void A_Destroy(void);
internal void A_SelectContext(A_State *state);

internal void A_SetFallback(A_Handle handle);

internal void A_PollHotReloads(void);
internal void A_FlushUploads(void);

internal A_Type A_GetAssetTypeFromPath(String8 path);

internal A_Handle A_HandleFromFilePath(String8 path);

internal A_Asset *A_GetOrFallback(A_Handle handle);
internal A_Asset *A_GetOrBreak(A_Handle handle);

typedef struct A_LoadJobParam A_LoadJobParam;
struct A_LoadJobParam
{
	Arena *arena;
	A_Record *record;
	OS_Handle counter;
};

internal J_ENTRY_POINT_DEF(A_LoadJob);

internal A_Handle A_RequireAsset(Arena *arena, String8 path, OS_Handle counter);
internal A_Handle A_RequireAssetBlocking(Arena *arena, String8 path);

internal void A_DestroyAsset(A_Handle handle);

internal void A_WaitForLoad(OS_Handle counter);
internal void A_WaitForLoadAndRelease(OS_Handle counter);


/* ==================================================
   FILESYSTEM
   ================================================== */

internal void A_Mount(String8 prefix, String8 directory);
internal String8 A_GetSystemFilePath(Arena *arena, String8 path);


#endif // ASSET_CORE_H
