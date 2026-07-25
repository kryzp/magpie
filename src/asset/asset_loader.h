#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

typedef struct A_Metadata A_Metadata;
struct A_Metadata
{
	String8 path;
	u64 last_write_time;
};

typedef struct A_ResidencyLevel A_ResidencyLevel;
struct A_ResidencyLevel
{
	u8 lod;
};

internal inline u32 A_ResidencyLevelCompare(A_ResidencyLevel a, A_ResidencyLevel b)
{
	return a.lod - b.lod;
}

typedef struct A_LoadResidencyCtx A_LoadResidencyCtx;
struct A_LoadResidencyCtx
{
	f32 distance;
};

typedef struct A_LCTX A_LCTX;
struct A_LCTX
{
	LOG_Channel log_channel;
	A_Metadata metadata;

	A_ResidencyLevel current_residency;
	A_ResidencyLevel target_residency;
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

// todo: rename to A_LoaderDesc or something as it consists
//       of an API + properties
typedef struct A_LoaderAPI A_LoaderAPI;
struct A_LoaderAPI
{
	b32 is_streamable;

	u32 file_extension_count;
	String8 *file_extensions;
	
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

	A_ResidencyLevel (*CalculateResidency)(const A_LoadResidencyCtx *ctx);
};

#endif // ASSET_LOADER_H
