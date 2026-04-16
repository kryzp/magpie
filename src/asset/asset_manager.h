#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

typedef struct AST_LoadData AST_LoadData;
struct AST_LoadData
{
	void *data;
	
	u64 stage_size;
	b32 failed;
	
	u32 dependency_count;
	AST_Handle *dependencies;

	u32 watch_path_count;
	String8 *watch_paths;
};

typedef struct AST_Upload AST_Upload;
struct AST_Upload
{
	Arena *arena;
	AST_MetaData metadata;
	AST_Handle handle;
	AST_Type type;
};

#define AST_UPLOAD_QUEUE_MAX_ELEMENTS 64

typedef struct AST_UploadQueue AST_UploadQueue;
struct AST_UploadQueue
{
	AST_Upload queue[AST_UPLOAD_QUEUE_MAX_ELEMENTS];
	u32 count;
};

typedef struct AST_Record AST_Record;
struct AST_Record
{
	AST_Asset asset;

	AST_Upload upload_data;

	String8 path;

	u64 last_write_time;

	u32 dependent_count;
	AST_Handle dependents[16];

	u32 watch_path_count;
	String8 *watch_paths;
};

typedef struct AST_HotRecord AST_HotRecord;
struct AST_HotRecord
{
	AST_State state;
	u32 generation;
	u32 pending_deps;
};

typedef struct AST_PathMapEntry AST_PathMapEntry;
struct AST_PathMapEntry
{
	char path[512];
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
	GFX_Device *graphics_device;

	u32 record_count;
	AST_Record records[512];
	AST_HotRecord hot_records[512];
	
	u32 free_index_count;
	u32 free_indices[512];

	AST_Serializer serializers[AST_Type_COUNT];

	AST_PathMapEntry path_map[512];

	AST_MountPoint mount_points[8];
	u32 mount_point_count;

	JOB_Counter *async_upload_counter;

	AST_UploadQueue upload_queue;
	OS_Handle       upload_mutex;

	AST_UploadQueue dependency_queue;
	OS_Handle       dependency_mutex;

	OS_Handle loading_mutex;
	OS_Handle loading_cond;

	OS_Handle allocation_mutex;
};


/* ==================================================
   CORE
   ================================================== */

internal void AST_Init(AST_Assets *assets, Arena *arena, GFX_Device *device);
internal void AST_Destroy(AST_Assets *assets);

internal void AST_PollHotReloads(AST_Assets *assets);
internal void AST_WaitForAsyncUploads(AST_Assets *assets);
internal void AST_FlushUploads(AST_Assets *assets);

internal u32 AST_AllocSlot(AST_Assets *assets);
internal AST_Record *AST_GetRecord(AST_Assets *assets, AST_Handle handle);


/* ==================================================
   UTILITIES
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

internal void AST_LoadDirectEx(AST_Assets *assets, AST_Handle handle, AST_Type type, JOB_Counter *counter);

internal void AST_PushUpload(AST_Assets *assets, const AST_Upload *upload);
internal void AST_PushForDependencyResolution(AST_Assets *assets, const AST_Upload *upload);


/* ==================================================
   ASSETS
   ================================================== */

internal AST_Asset *AST_AssetCreate  (AST_Assets *assets, String8 path);
internal void       AST_AssetDestroy (AST_Assets *assets, AST_Handle handle);
internal AST_Asset *AST_AssetGet     (AST_Assets *assets, AST_Handle handle, AST_Type type);
internal void       AST_AssetUnload  (AST_Assets *assets, AST_Asset *asset);


/* ==================================================
   HANDLES
   ================================================== */

internal AST_Handle AST_FromFilePath(AST_Assets *assets, String8 path);


#endif // ASSET_MANAGER_H
