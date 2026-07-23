#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

typedef struct A_Metadata A_Metadata;
struct A_Metadata
{
	String8 path;
	u64 last_write_time;
};

typedef struct A_LCTX A_LCTX;
struct A_LCTX
{
	LOG_Channel log_channel;
	A_Metadata metadata;
};

typedef struct A_LoadResult A_LoadResult;
struct A_LoadResult
{
	void *user_data;
	u64 stage_size;
	b32 failed;
	A_Handle *dependencies;
	u32 dependency_count;
};

typedef struct A_LoaderAPI A_LoaderAPI;
struct A_LoaderAPI
{
	A_LoadResult (*Load)(const A_LCTX *ctx,
						 Arena *result_arena);
	
	void (*Alloc)(const A_LCTX *ctx,
				  A_LoadResult *result,
				  Arena *asset_arena,
				  A_Asset *asset);
	
	void (*UploadGPU)(const A_LCTX *ctx,
					  A_LoadResult *result,
					  A_Asset *asset,
					  G_CmdBuffer *cmd,
					  G_ResourceKey stage,
					  u64 stage_offset);

	void (*DestroyIntermediateResources)(A_LoadResult *result);

	void (*DestroyAsset)(A_Asset *asset);
};

#endif // ASSET_LOADER_H
