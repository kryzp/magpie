
typedef struct AST_ShaderLoadData AST_ShaderLoadData;
struct AST_ShaderLoadData
{
	GFX_ShaderCompiledStages compiled;
};

internal AST_SerializerPipelineData
AST_ShaderSerializerCpu(const AST_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);
	
	AST_ShaderLoadData *shader_data = ArenaPushArray(load_scope, AST_ShaderLoadData, 1);
	
	AST_SerializerPipelineData result = {0};
	result.data = shader_data;
	result.stage_size = 0;
	result.failed = false;
	result.dependency_count = 0;

	u64 shader_index = String8Find(file_path, String8Lit("shaders"));

	String8 search_directory = String8Append(scratch.arena,
											 String8Substr(file_path, 0, shader_index),
											 String8Lit("shaders/modules/"));

	GFX_ShaderCompiler *compiler = ctx->assets->shader_compiler;

	shader_data->compiled = GFX_ShaderCompilerCompile(compiler,
													  load_scope,
													  file_path,
													  1, &search_directory);

	result.failed = shader_data->compiled.failed;
	
	result.watch_path_count = 2;
	
	result.watch_paths = ArenaPushArray(load_scope, String8, result.watch_path_count);
	result.watch_paths[0] = file_path;
	result.watch_paths[1] = search_directory;

	ScratchRelease(&scratch);
	
	return result;
}

internal void
AST_ShaderSerializerAlloc(const AST_Context *ctx,
						  AST_SerializerPipelineData *data,
						  AST_Asset *out,
						  Arena *arena)
{
	AST_ShaderLoadData *shader_data = data->data;

	out->shader.key = GFX_DeviceShaderProgramCreate(ctx->assets->device,
													shader_data->compiled.count,
													shader_data->compiled.bytecodes);
}

internal void
AST_ShaderSerializerReload(const AST_Context *ctx,
						   AST_SerializerPipelineData *data,
						   AST_Asset *existing)
{
	AST_ShaderLoadData *shader_data = data->data;

	GFX_DeviceShaderProgramDestroy(ctx->assets->device, existing->shader.key);
	
	existing->shader.key = GFX_DeviceShaderProgramCreate(ctx->assets->device,
														 shader_data->compiled.count,
														 shader_data->compiled.bytecodes);
}

internal void
AST_ShaderSerializerDispose(AST_Asset *asset, AST_Assets *assets)
{
	GFX_DeviceShaderProgramDestroy(assets->device, asset->shader.key);
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
