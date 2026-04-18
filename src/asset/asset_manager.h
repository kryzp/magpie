#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#define AST_MANAGER_MAX_RECORDS 512
#define AST_GPU_UPLOAD_CHUNK    Megabytes(128)

#define AST_LOAD_ARENA_COUNT 16
#define AST_LOAD_ARENA_SIZE  Megabytes(64)

typedef struct AST_Record AST_Record;
struct AST_Record
{
	AST_Asset asset;
	AST_State state;

	u32 generation;
	
	AST_Upload stashed_upload;

	String8 path;

	u64 last_write_time;

	u32        dependent_count;
	AST_Handle dependents[16];

	u32      watch_path_count;
	String8 *watch_paths;

	u32 hot_pending_dependencies;
};

typedef struct AST_PathMapEntry AST_PathMapEntry;
struct AST_PathMapEntry
{
	String8 path;
	AST_Handle value;
	b32 occupied;
};

typedef struct AST_MountPoint AST_MountPoint;
struct AST_MountPoint
{
	String8 prefix;
	String8 directory;
};

typedef struct AST_Assets AST_Assets;
struct AST_Assets
{
	Arena *arena;

	GFX_Device *device;
	const AUD_BackendAPI *audio_backend;

	u32 record_count;
	AST_Record records[AST_MANAGER_MAX_RECORDS];
	
	u32 free_index_count;
	u32 free_indices[AST_MANAGER_MAX_RECORDS];

	AST_Serializer serializers[AST_Type_COUNT];

	AST_PathMapEntry path_map[AST_MANAGER_MAX_RECORDS];

	AST_MountPoint mount_points[8];
	u32 mount_point_count;

	JOB_Counter *async_upload_counter;

	// todo: move mutex into upload queue
	AST_UploadQueue upload_queue;
	OS_Handle       upload_mutex;

	AST_UploadQueue dependency_queue;
	OS_Handle       dependency_mutex;

	OS_Handle loading_mutex;
	OS_Handle loading_cond;

	OS_Handle allocation_mutex;

	Arena load_arenas[AST_LOAD_ARENA_COUNT];
	u32 free_load_arenas[AST_LOAD_ARENA_COUNT];
	u32 free_load_arena_count;
	u32 load_arena_spinlock;
};


/* ==================================================
   HELPERS
   ================================================== */

internal u64 AST_FindSchemeSeparator(String8 path);

internal u32 AST_AllocSlot(AST_Assets *assets);
internal void AST_FreeSlot(AST_Assets *assets, u32 index);

internal AST_Handle  AST_AllocRecord(AST_Assets *assets, String8 path);
internal AST_Record *AST_GetRecord(AST_Assets *assets, AST_Handle handle);
internal const AST_Record *AST_GetRecordConst(const AST_Assets *assets, AST_Handle handle);

internal AST_Handle AST_PathMapFind(const AST_Assets *assets, String8 path);
internal void       AST_PathMapInsert(AST_Assets *assets, String8 path, AST_Handle handle);

internal u32  AST_LoadArenaAcquire(AST_Assets *assets);
internal void AST_LoadArenaRelease(AST_Assets *assets, u32 index);


/* ==================================================
   CORE
   ================================================== */

internal void AST_Init(AST_Assets *assets, Arena *arena, GFX_Device *device, const AUD_BackendAPI *audio_backend);
internal void AST_Destroy(AST_Assets *assets);


/* ==================================================
   FILESYSTEM
   ================================================== */

internal void AST_Mount(AST_Assets *assets, String8 prefix, String8 directory);
internal String8 AST_GetSystemFilePath(AST_Assets *assets, Arena *arena, String8 path);


/* ==================================================
   QUERY
   ================================================== */

internal b32 AST_IsLoaded  (const AST_Assets *assets, AST_Handle handle);
internal b32 AST_IsLoading (const AST_Assets *assets, AST_Handle handle);
internal b32 AST_IsValid   (const AST_Assets *assets, AST_Handle handle);


/* ==================================================
   LOADING
   ================================================== */

internal void AST_LoadNow     (AST_Assets *assets, AST_Handle handle, AST_Type type);
internal void AST_LoadAsync   (AST_Assets *assets, AST_Handle handle, AST_Type type);
internal void AST_ReloadAsync (AST_Assets *assets, AST_Handle handle, AST_Type type);
internal void AST_Load        (AST_Assets *assets, AST_Handle handle, AST_Type type, JOB_Counter *counter);

internal void AST_NotifyDependents       (AST_Assets *assets, AST_Handle handle);
internal void AST_NotifyDependentsNoLock (AST_Assets *assets, AST_Handle handle, b32 failed);

internal void AST_ResolvePendingDependencies(AST_Assets *assets, JOB_Counter *counter);


typedef struct AST_LoadJobParam AST_LoadJobParam;
struct AST_LoadJobParam
{
	AST_Assets *assets;
	AST_MetaData metadata;
	AST_Handle handle;
	AST_Type type;
};

JOB_ENTRY_POINT_DEF(AST_LoadJobEntry);


internal void AST_PollHotReloads(AST_Assets *assets);
internal void AST_FlushUploads(AST_Assets *assets);
internal void AST_WaitForAsyncUploads(AST_Assets *assets);


/* ==================================================
   ASSETS
   ================================================== */

// TODO: Functionality to add/create and remove/unload assets directly via code.


/* ==================================================
   HANDLES
   ================================================== */

internal AST_Handle AST_FromFilePath (AST_Assets *assets, String8 path);
internal AST_Handle AST_Require      (AST_Assets *assets, String8 path, AST_Type type); // ensure it's loading


#endif // ASSET_MANAGER_H
