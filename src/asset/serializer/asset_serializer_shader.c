
typedef struct A_ShaderLoadData A_ShaderLoadData;
struct A_ShaderLoadData
{
	G_ShaderCompiledStages compiled;
};

static A_SerializerPipelineData A_ShaderSerializerCpu(const A_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = A_ContextSystemFilePath(ctx, scratch.arena);
	
	A_ShaderLoadData *shader = ArenaPushArray(load_scope, A_ShaderLoadData, 1);
	
	A_SerializerPipelineData result = {0};
	result.data = shader;
	result.stage_size = 0;
	result.failed = false;
	result.dependency_count = 0;

	u64 shader_index = String8Find(file_path, String8Lit("shaders"));

	String8 search_directory = String8Append(scratch.arena,
											 String8Substr(file_path, 0, shader_index),
											 String8Lit("shaders/modules/"));

	G_ShaderCompiler *compiler = ctx->assets->shader_compiler;

	shader->compiled = G_ShaderCompilerCompile(compiler,
													  load_scope,
													  file_path,
													  1, &search_directory);

	result.failed = shader->compiled.failed;
	
	result.watch_path_count = 2;
	
	result.watch_paths = ArenaPushArray(load_scope, String8, result.watch_path_count);
	result.watch_paths[0] = file_path;
	result.watch_paths[1] = search_directory;

	ScratchRelease(&scratch);
	
	return result;
}

static void A_ShaderSerializerAlloc(const A_Context *ctx,
						  A_SerializerPipelineData *data,
						  A_Asset *out,
						  Arena *arena)
{
	A_ShaderLoadData *shader = data->data;

	out->shader.key = G_DeviceShaderProgramCreate(ctx->assets->device,
														 shader->compiled.count,
														 shader->compiled.bytecodes);
}

static void A_ShaderSerializerReload(const A_Context *ctx,
						   A_SerializerPipelineData *data,
						   A_Asset *existing)
{
	A_ShaderLoadData *shader = data->data;

	G_DeviceShaderProgramDestroy(ctx->assets->device, existing->shader.key);
	
	existing->shader.key = G_DeviceShaderProgramCreate(ctx->assets->device,
															  shader->compiled.count,
															  shader->compiled.bytecodes);
}

static void A_ShaderSerializerDispose(A_Asset *asset, A_Registry *assets)
{
	G_DeviceShaderProgramDestroy(assets->device, asset->shader.key);
}

static A_Serializer A_GetShaderSerializer(void)
{
	static A_Serializer shader_serializer = {
		.Cpu     = A_ShaderSerializerCpu,
		.Alloc   = A_ShaderSerializerAlloc,
		.Reload  = A_ShaderSerializerReload,
		.Gpu     = NULL,
		.End     = NULL,
		.Dispose = A_ShaderSerializerDispose,
	};

	return shader_serializer;
}
