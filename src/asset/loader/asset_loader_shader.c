
typedef struct A_ShaderLoadData A_ShaderLoadData;
struct A_ShaderLoadData
{
	G_ShaderCompiledStages compiled;
};

internal A_LoadResult A_ShaderLoaderLoad(const A_LCTX *ctx,
									   Arena *result_arena)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = A_GetSystemFilePath(scratch.arena, ctx->metadata.path);
	
	u64 shader_index = String8Find(file_path, String8Lit("shaders"));

	String8 search_directory = String8Append(scratch.arena,
											 String8Substr(file_path, 0, shader_index),
											 String8Lit("shaders/modules/"));

	A_ShaderLoadData *load_data = ArenaPushArray(result_arena, A_ShaderLoadData, 1);

	load_data->compiled = G_ShaderCompilerCompile(result_arena,
												  file_path,
												  1, &search_directory);

	A_LoadResult result = {0};
	result.user_data = load_data;
	result.stage_size = 0;
	result.failed = load_data->compiled.failed;

	ScratchRelease(&scratch);
	
	return result;
}

internal void A_ShaderLoaderAlloc(const A_LCTX *ctx,
								A_LoadResult *result,
								Arena *asset_arena,
								A_Asset *asset)
{
	A_ShaderLoadData *load_data = result->user_data;

	G_ShaderCompiledStages *compiled = &load_data->compiled;
	
	asset->shader.key = G_ShaderProgramCreate(compiled->count, compiled->bytecodes);
}

internal void A_ShaderLoaderDestroyAsset(A_Asset *asset)
{
	G_ShaderProgramDestroy(asset->shader.key);
}

internal b32 A_ShaderLoaderIsAssetMine(String8 extension)
{
	return String8Match(extension, String8Lit("slang"));
}

internal A_LoaderAPI A_GetShaderLoaderAPI(void)
{
	static A_LoaderAPI shader_loader_api = {
		.Load = A_ShaderLoaderLoad,
		.Alloc = A_ShaderLoaderAlloc,
		.UploadGPU = NULL,
		.DestroyIntermediateResources = NULL,
		.DestroyAsset = A_ShaderLoaderDestroyAsset,
		.IsAssetMine = A_ShaderLoaderIsAssetMine
	};

	return shader_loader_api;
}
