#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#define A_MANAGER_MAX_RECORDS     512
#define A_GPU_UPLOAD_CHUNK        Megabytes(128)

#define A_LOAD_ARENA_COUNT        128
#define A_LOAD_ARENA_RESERVE      Gigabytes(1)

typedef enum A_RecordState
{
	A_RecordState_Unloaded,
	A_RecordState_CpuStage,
	A_RecordState_WaitingForDependencies,
	A_RecordState_GpuStage,
	A_RecordState_Ready,
	A_RecordState_Failed,
	A_RecordState_COUNT
}
A_RecordState;

typedef struct A_Record A_Record;
struct A_Record
{
	A_Asset asset;
	
	A_RecordState state;

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

typedef struct A_Assets A_Assets;
struct A_Assets
{
	Arena *arena;
	LOG_Channel log_channel;

	u32 record_count;
	A_Record records[A_MANAGER_MAX_RECORDS];
	
	A_Handle fallbacks[A_Type_COUNT];
	A_Asset null_asset_sentinel; // if a fallback doesn't exist for this asset
	
	u32 free_index_count;
	u32 free_indices[A_MANAGER_MAX_RECORDS];

	A_Serializer serializers[A_Type_COUNT];
	LOG_Channel serializer_log_channels[A_Type_COUNT];

	A_PathMapEntry path_map[A_MANAGER_MAX_RECORDS];

	A_MountPoint mount_points[8];
	u32 mount_point_count;

	OS_Handle async_counter;

	// todo: move mutex into upload queue
	A_UploadQueue upload_queue;
	OS_Handle upload_mutex;

	A_UploadQueue dependency_queue;
	OS_Handle dependency_mutex;

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

static u64 A_FindSchemeSeparator(String8 path);

static u32 A_AllocSlot(void);
static void A_FreeSlot(u32 index);

static A_Handle A_AllocRecord(String8 path);
static A_Record *A_GetRecord(A_Handle handle);
static const A_Record *A_GetRecordConst(A_Handle handle);

static A_Handle A_PathMapFind(String8 path);
static void A_PathMapInsert(String8 path, A_Handle handle);

static u32 A_LoadArenaAcquire(void);
static void A_LoadArenaRelease(u32 index);


/* ==================================================
   CORE
   ================================================== */

static void A_InitAndSelect(A_Assets *assets, Arena *arena, LOG_Channel log_channel);
static void A_Destroy(void);
static void A_SelectContext(A_Assets *assets);


/* ==================================================
   FILESYSTEM
   ================================================== */

static void A_Mount(String8 prefix, String8 directory);
static String8 A_GetSystemFilePath(Arena *arena, String8 path);


/* ==================================================
   QUERY
   ================================================== */

static b32 A_IsLoaded(A_Handle handle);
static b32 A_IsLoading(A_Handle handle);
static b32 A_IsValid(A_Handle handle);


/* ==================================================
   LOADING
   ================================================== */

static void A_LoadNow(A_Handle handle);
static void A_LoadAsync(A_Handle handle);
static void A_ReloadAsync(A_Handle handle);
static void A_Load(A_Handle handle, OS_Handle counter);

static void A_NotifyDependents(A_Handle handle);
static void A_NotifyDependentsNoLock(A_Handle handle, b32 failed);

static void A_ResolvePendingDependencies(OS_Handle counter);


typedef struct A_LoadJobParam A_LoadJobParam;
struct A_LoadJobParam
{
	A_MetaData metadata;
	A_Handle handle;
};

static J_ENTRY_POINT_DEF(A_LoadJobEntry);


static void A_PollHotReloads(void);
static void A_FlushUploads(void);
static void A_WaitForAsync(void);
static void A_WaitForLoad(A_Handle handle, OS_Handle counter);


/* ==================================================
   ASSETS
   ================================================== */

static void A_SetFallback(A_Handle handle, A_Type type);

static A_Asset *A_Get(A_Handle handle);
static A_Asset *A_GetNow(A_Handle handle); // block until we got it.


/* ==================================================
   HANDLES
   ================================================== */

static A_Handle A_FromFilePath(String8 path, A_Type type);
static A_Handle A_Require(String8 path, A_Type type); // ensure it's loading.


#endif // ASSET_MANAGER_H
