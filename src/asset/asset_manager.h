#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

typedef enum AST_Type
{
	AST_Type_Unknown,
#define AssetDef(name) AST_Type_##name,
#include "asset_definitions.inc"
#undef AssetDef
	AST_Type_COUNT
}
AST_Type;

internal inline AST_Type
AST_TypeFromString(String8 str)
{
	// TODO
	
	AssertTrue(false);
}

internal inline String8
AST_StringFromType(Arena *arena, AST_Type type)
{
	// TODO
	
	AssertTrue(false);
}

typedef enum AST_State
{
	AST_State_Unloaded,
	AST_State_CpuStage,
	AST_State_WaitingForDependencies,
	AST_State_GpuStage,
	AST_State_Ready,
	AST_State_Failed,
	AST_State_COUNT
}
AST_State;

internal inline b32
AST_StateIsLoading(AST_State st)
{
	// TODO
	
	AssertTrue(false);
}

internal inline b32
AST_StateNeedsLoad(AST_State st)
{
	// TODO
	
	AssertTrue(false);
}

internal inline b32
AST_StateIsLoaded(AST_State st)
{
	// TODO
	
	AssertTrue(false);
}

internal inline b32
AST_StateIsFinalized(AST_State st)
{
	// TODO
	
	AssertTrue(false);
}

typedef struct AST_MetaData AST_MetaData;
struct AST_MetaData
{
	String8 path;
};

typedef struct AST_Handle AST_Handle;
struct AST_Handle
{
	u32 index;
	u32 generation;
};

internal inline AST_Handle
AST_HandleNull(void)
{
	AST_Handle handle = {0};
	handle.index = -1u;
	handle.generation = 0;

	return handle;
}

internal inline b32
AST_HandleMatch(AST_Handle a, AST_Handle b)
{
	return (a.index == b.index &&
			a.generation == b.generation);
}

typedef struct AST_Asset AST_Asset;
struct AST_Asset
{
	AST_Type type;
	AST_Handle handle;

	/*
	union
	{
		struct
		{
		}
		texture;

		struct
		{
		}
		shader;

		struct
		{
		}
		model;

		struct
		{
		}
		sound;
	};
	*/
};

typedef struct AST_Assets AST_Assets;

typedef struct AST_Context AST_Context;
struct AST_Context
{
	Arena *scratch;
	AST_Assets *assets;
	AST_MetaData metadata;
};

internal String8 AST_ContextSystemFilePath(const AST_Context *context);

typedef struct AST_LoadData AST_LoadData;
struct AST_LoadData
{
	void *data;
	
	u64 stage_size;
	b32 failed;
	
	u32 dependency_count;
	AST_Handle dependencies[16];

	u32 watch_path_count;
	char watch_paths[16][512];
};

typedef struct AST_Upload AST_Upload;
struct AST_Upload
{
	Arena *arena;
	AST_MetaData metadata;
	AST_Handle handle;
	AST_Type type;
};

typedef struct AST_UploadQueue AST_UploadQueue;
struct AST_UploadQueue
{
};

typedef struct AST_Serializer AST_Serializer;
struct AST_Serializer
{
	AST_LoadData *(*Load)(const AST_Context *ctx);
	void (*Finalize)(const AST_Context *ctx, void *data, AST_Asset *existing, GFX_Device *device);
	void (*Upload)(const AST_Context *ctx, void *data, AST_Asset *asset, GFX_CmdBuffer *cmd, GFX_Buffer *stage, u64 stage_base);
	void (*Dispose)(void *data);
};

// TODO: this is a bitch
typedef struct AST_Record AST_Record;
struct AST_Record
{
	AST_Asset *asset;

	AST_State state;

	AST_Upload upload_data;
	
	u32 generation;
	
	String8 path;

	u64 last_write_time;
	
	u32 pending_dependencies;
	
	u32 dependent_count;
	AST_Handle dependents[16];

	u32 watch_path_count;
	char watch_paths[16][512];
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
	char prefix[64];
	char directory[512];
};

typedef struct AST_Assets AST_Assets;
struct AST_Assets
{
	GFX_Device *graphics_device;
	
	AST_Record records[512];
	
	u32 free_indices[512];
	u32 free_index_count;

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

internal void AST_Init(AST_Assets *assets, GFX_Device *device);
internal void AST_Destroy(AST_Assets *assets);

internal void AST_PollHotReloads(AST_Assets *assets);
internal void AST_WaitForAsyncUploads(AST_Assets *assets);
internal void AST_FlushUploads(AST_Assets *assets);


/* ==================================================
   UTILITIES
   ================================================== */

internal void AST_Mount(AST_Assets *assets, String8 prefix, String8 directory);
internal String8 AST_GetSystemFilePath(AST_Assets *assets, Arena *arena, String8 path);


/* ==================================================
   QUERY
   ================================================== */

internal b32 AST_IsLoaded(const AST_Assets *assets, AST_Handle handle);
internal b32 AST_IsLoading(const AST_Assets *assets, AST_Handle handle);
internal b32 AST_IsValid(const AST_Assets *assets, AST_Handle handle);


/* ==================================================
   LOADING
   ================================================== */

internal void AST_LoadNow(AST_Assets *assets, AST_Handle handle, AST_Type type);
internal void AST_LoadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type);
internal void AST_ReloadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type);

internal void AST_LoadDirectEx(AST_Assets *assets, AST_Handle handle, AST_Type type, JOB_Counter *counter);

internal void AST_PushUpload(AST_Assets *assets, const AST_Upload *upload);
internal void AST_PushForDependencyResolution(AST_Assets *assets, const AST_Upload *upload);


/* ==================================================
   ASSETS
   ================================================== */

internal AST_Asset *AST_AssetCreate(AST_Assets *assets, String8 path);
internal void       AST_AssetDestroy(AST_Handle handle);
internal AST_Asset *AST_AssetGet(AST_Assets *assets, AST_Handle handle, AST_Type type);
internal void       AST_AssetUnload(AST_Asset *asset);


/* ==================================================
   HANDLES
   ================================================== */

internal AST_Handle AST_FromFilePath(AST_Assets *assets, String8 path);


#endif // ASSET_MANAGER_H
