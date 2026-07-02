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

static u64 A_FindSchemeSeparator(String8 path);

static u32 A_AllocSlot(A_Assets *assets);
static void A_FreeSlot(A_Assets *assets, u32 index);

static A_Handle A_AllocRecord(A_Assets *assets, String8 path);
static A_Record *A_GetRecord(A_Assets *assets, A_Handle handle);
static const A_Record *A_GetRecordConst(const A_Assets *assets, A_Handle handle);

static A_Handle A_PathMapFind(const A_Assets *assets, String8 path);
static void A_PathMapInsert(A_Assets *assets, String8 path, A_Handle handle);

static u32 A_LoadArenaAcquire(A_Assets *assets);
static void A_LoadArenaRelease(A_Assets *assets, u32 index);


/* ==================================================
   CORE
   ================================================== */

static void A_Init(A_Assets *assets, Arena *arena, LOG_Channel log_channel,
					 G_Device *device,
					 G_ShaderCompiler *shader_compiler,
					 AU_Backend *audio_backend,
					 S_System *scripting_system);

static void A_Destroy(A_Assets *assets);


/* ==================================================
   FILESYSTEM
   ================================================== */

static void A_Mount(A_Assets *assets, String8 prefix, String8 directory);
static String8 A_GetSystemFilePath(A_Assets *assets, Arena *arena, String8 path);


/* ==================================================
   QUERY
   ================================================== */

static b32 A_IsLoaded  (const A_Assets *assets, A_Handle handle);
static b32 A_IsLoading (const A_Assets *assets, A_Handle handle);
static b32 A_IsValid   (const A_Assets *assets, A_Handle handle);


/* ==================================================
   LOADING
   ================================================== */

static void A_LoadNow     (A_Assets *assets, A_Handle handle);
static void A_LoadAsync   (A_Assets *assets, A_Handle handle);
static void A_ReloadAsync (A_Assets *assets, A_Handle handle);
static void A_Load        (A_Assets *assets, A_Handle handle, OS_Handle counter);

static void A_NotifyDependents       (A_Assets *assets, A_Handle handle);
static void A_NotifyDependentsNoLock (A_Assets *assets, A_Handle handle, b32 failed);

static void A_ResolvePendingDependencies(A_Assets *assets, OS_Handle counter);


typedef struct A_LoadJobParam A_LoadJobParam;
struct A_LoadJobParam
{
	A_Assets *assets;
	A_MetaData metadata;
	A_Handle handle;
};

static J_ENTRY_POINT_DEF(A_LoadJobEntry);


static void A_PollHotReloads (A_Assets *assets);
static void A_FlushUploads   (A_Assets *assets);
static void A_WaitForAsync   (A_Assets *assets);
static void A_WaitForLoad    (A_Assets *assets, A_Handle handle, OS_Handle counter);


/* ==================================================
   ASSETS
   ================================================== */

static void     A_SetFallback (A_Assets *assets, A_Handle handle, A_Type type);

static A_Asset *A_Get         (A_Assets *assets, A_Handle handle);
static A_Asset *A_GetNow      (A_Assets *assets, A_Handle handle); // block until we got it.


/* ==================================================
   HANDLES
   ================================================== */

static A_Handle A_FromFilePath (A_Assets *assets, String8 path, A_Type type);
static A_Handle A_Require      (A_Assets *assets, String8 path, A_Type type); // ensure it's loading.


#endif // ASSET_MANAGER_H
