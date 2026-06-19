#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#define A_MANAGER_MAX_RECORDS 512
#define A_GPU_UPLOAD_CHUNK    Megabytes(128)

#define A_LOAD_ARENA_COUNT    128
#define A_LOAD_ARENA_RESERVE  Gigabytes(1)

typedef struct A_Record A_Record;
struct A_Record
{
	A_Asset asset;
	
	A_State state;

	u32 generation;
	
	A_Upload stashed_upload;

	String8 path;

	u64 last_write_time;

	u32 dependent_count;
	A_Handle dependents[16];

	u32 watch_path_count;
	String8 *watch_paths;

	u32 hot_pending_dependencies;

	b32 reloading;
};

typedef struct A_PathMapEntry A_PathMapEntry;
struct A_PathMapEntry
{
	String8 path;
	A_Handle value;
	b32 occupied;
};

typedef struct A_MountPoint A_MountPoint;
struct A_MountPoint
{
	String8 prefix;
	String8 directory;
};

typedef struct A_Registry A_Registry;
struct A_Registry
{
	Arena *arena;

	LOG_Channel log_channel;

	G_Device *device;
	G_ShaderCompiler *shader_compiler;
	AU_Backend *audio_backend;
	S_System *scripting_system;

	u32 record_count;
	A_Record records[A_MANAGER_MAX_RECORDS];
	
	A_Handle fallbacks[A_Type_COUNT];
	A_Asset null_asset_sentinel; // if a fallback doesn't exist for this asset
	
	u32 free_index_count;
	u32 free_indices[A_MANAGER_MAX_RECORDS];

	A_Serializer serializers[A_Type_COUNT];
	LOG_Channel    serializer_log_channels[A_Type_COUNT];

	A_PathMapEntry path_map[A_MANAGER_MAX_RECORDS];

	A_MountPoint mount_points[8];
	u32 mount_point_count;

	OS_Handle async_counter;

	// todo: move mutex into upload queue
	A_UploadQueue upload_queue;
	OS_Handle       upload_mutex;

	A_UploadQueue dependency_queue;
	OS_Handle       dependency_mutex;

	OS_Handle loading_mutex;
	OS_Handle loading_cond;

	OS_Handle allocation_mutex;

	Arena load_arenas[A_LOAD_ARENA_COUNT];
	u32 free_load_arenas[A_LOAD_ARENA_COUNT];
	u32 free_load_arena_count;
	u32 load_arena_spinlock;
	OS_Handle load_arena_wait_counter;
};


/* ==================================================
   HELPERS
   ================================================== */

internal u64 A_FindSchemeSeparator(String8 path);

internal u32 A_AllocSlot(A_Registry *assets);
internal void A_FreeSlot(A_Registry *assets, u32 index);

internal A_Handle A_AllocRecord(A_Registry *assets, String8 path);
internal A_Record *A_GetRecord(A_Registry *assets, A_Handle handle);
internal const A_Record *A_GetRecordConst(const A_Registry *assets, A_Handle handle);

internal A_Handle A_PathMapFind(const A_Registry *assets, String8 path);
internal void A_PathMapInsert(A_Registry *assets, String8 path, A_Handle handle);

internal u32 A_LoadArenaAcquire(A_Registry *assets);
internal void A_LoadArenaRelease(A_Registry *assets, u32 index);


/* ==================================================
   CORE
   ================================================== */

internal void A_Init(A_Registry *assets, Arena *arena, LOG_Channel log_channel,
					 G_Device *device,
					 G_ShaderCompiler *shader_compiler,
					 AU_Backend *audio_backend,
					 S_System *scripting_system);

internal void A_Destroy(A_Registry *assets);


/* ==================================================
   FILESYSTEM
   ================================================== */

internal void A_Mount(A_Registry *assets, String8 prefix, String8 directory);
internal String8 A_GetSystemFilePath(A_Registry *assets, Arena *arena, String8 path);


/* ==================================================
   QUERY
   ================================================== */

internal b32 A_IsLoaded  (const A_Registry *assets, A_Handle handle);
internal b32 A_IsLoading (const A_Registry *assets, A_Handle handle);
internal b32 A_IsValid   (const A_Registry *assets, A_Handle handle);


/* ==================================================
   LOADING
   ================================================== */

internal void A_LoadNow     (A_Registry *assets, A_Handle handle, A_Type type);
internal void A_LoadAsync   (A_Registry *assets, A_Handle handle, A_Type type);
internal void A_ReloadAsync (A_Registry *assets, A_Handle handle, A_Type type);
internal void A_Load        (A_Registry *assets, A_Handle handle, A_Type type, OS_Handle counter);

internal void A_NotifyDependents       (A_Registry *assets, A_Handle handle);
internal void A_NotifyDependentsNoLock (A_Registry *assets, A_Handle handle, b32 failed);

internal void A_ResolvePendingDependencies(A_Registry *assets, OS_Handle counter);


typedef struct A_LoadJobParam A_LoadJobParam;
struct A_LoadJobParam
{
	A_Registry *assets;
	A_MetaData metadata;
	A_Handle handle;
	A_Type type;
};

J_ENTRY_POINT_DEF(A_LoadJobEntry);


internal void A_PollHotReloads (A_Registry *assets);
internal void A_FlushUploads   (A_Registry *assets);
internal void A_WaitForAsync   (A_Registry *assets);
internal void A_WaitForLoad    (A_Registry *assets, A_Handle handle, OS_Handle counter);


/* ==================================================
   ASSETS
   ================================================== */

internal void     A_SetFallback (A_Registry *assets, A_Handle handle, A_Type type);

internal A_Asset *A_Get         (A_Registry *assets, A_Handle handle, A_Type type);
internal A_Asset *A_GetNow      (A_Registry *assets, A_Handle handle, A_Type type); // block until we got it.


/* ==================================================
   HANDLES
   ================================================== */

internal A_Handle A_FromFilePath (A_Registry *assets, String8 path);
internal A_Handle A_Require      (A_Registry *assets, String8 path, A_Type type); // ensure it's loading.


#endif // ASSET_MANAGER_H
