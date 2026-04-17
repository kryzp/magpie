
typedef struct AST_ShaderLoadData AST_ShaderLoadData;
struct AST_ShaderLoadData
{
	GFX_ShaderCompiledStages compiled;
};

internal AST_LoadData
AST_ShaderSerializerCpu(const AST_Context *ctx)
{
	ScratchArena scratch = ScratchBegin(&ctx->scope, 1);
	
	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);
	
	AST_ShaderLoadData *shader_data = ArenaPushArray(ctx->scope, AST_ShaderLoadData, 1);
	
	AST_LoadData result = {0};
	result.data = shader_data;
	result.stage_size = 0;
	result.failed = false;
	result.dependency_count = 0;

	u64 shader_index = String8Find(file_path, Str8("shaders"));

	String8 search_directory = String8Append(scratch.arena,
											 String8Substr(file_path, 0, shader_index),
											 Str8("shaders/modules/"));

	const GFX_ShaderCompilerAPI *api = GFX_GetShaderCompilerAPI();

	shader_data->compiled = api->Compile(ctx->scope,
										 file_path,
										 1, &search_directory);

	result.failed = shader_data->compiled.failed;
	
	result.watch_path_count = 2;
	
	result.watch_paths = ArenaPushArray(ctx->scope, String8, result.watch_path_count);
	result.watch_paths[0] = file_path;
	result.watch_paths[1] = search_directory;

	ScratchRelease(&scratch);
	
	return result;
}

internal void
AST_ShaderSerializerAlloc(const AST_Context *ctx,
						  AST_LoadData *data,
						  GFX_Device *device,
						  AST_Asset *out)
{
	AST_ShaderLoadData *shader_data = data->data;

	out->shader.key = GFX_DeviceShaderProgramCreate(device,
													shader_data->compiled.count,
													shader_data->compiled.bytecodes);
}

internal void
AST_ShaderSerializerReload(const AST_Context *ctx,
						   AST_LoadData *data,
						   GFX_Device *device,
						   AST_Asset *existing)
{
	// TODO
}

internal void
AST_ShaderSerializerDispose(AST_Asset *asset, GFX_Device *device)
{
	GFX_DeviceShaderProgramDestroy(device, asset->shader.key);
}

internal AST_Serializer
AST_GetShaderSerializer(void)
{
	static AST_Serializer shader_serializer = {
		.Cpu     = AST_ShaderSerializerCpu,
		.Alloc   = AST_ShaderSerializerAlloc,
		.Reload  = AST_ShaderSerializerReload,
		.Gpu     = NULL,
		.End     = NULL,
		.Dispose = AST_ShaderSerializerDispose,
	};

	return shader_serializer;
}
