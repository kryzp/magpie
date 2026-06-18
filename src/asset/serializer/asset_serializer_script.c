
typedef struct AST_ScriptLoadData AST_ScriptLoadData;
struct AST_ScriptLoadData
{
	SCR_ScriptRef ref;
};

internal AST_SerializerPipelineData
AST_ScriptSerializerCpu(const AST_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);

	AST_ScriptLoadData *script_data = ArenaPushArray(load_scope, AST_ScriptLoadData, 1);
	script_data->ref = SCR_ScriptRefNull();
	
	IO_ByteSpan source_bytes = IO_ReadEntireFile(scratch.arena, file_path);
	SCR_ScriptRef chunk_ref = SCR_Compile(ctx->assets->scripting_system, source_bytes, file_path);

	if (!SCR_ScriptRefIsNull(chunk_ref))
	{
		script_data->ref = SCR_ExecuteModule(ctx->assets->scripting_system, chunk_ref);
		SCR_Release(ctx->assets->scripting_system, chunk_ref);
	}
	
	AST_SerializerPipelineData result = {0};
	result.data = script_data;
	result.stage_size = 0;
	result.failed = false;
	result.dependency_count = 0;

	ScratchRelease(&scratch);
	
	return result;
}

internal void
AST_ScriptSerializerAlloc(const AST_Context *ctx,
						 AST_SerializerPipelineData *data,
						 AST_Asset *out,
						 Arena *arena)
{
	AST_ScriptLoadData *script_data = data->data;

	out->script_data.ref = script_data->ref;
}

internal void
AST_ScriptSerializerReload(const AST_Context *ctx,
						  AST_SerializerPipelineData *data,
						  AST_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

internal AST_Serializer
AST_GetScriptSerializer(void)
{
	static AST_Serializer script_serializer = {
		.Cpu     = AST_ScriptSerializerCpu,
		.Alloc   = AST_ScriptSerializerAlloc,
		.Reload  = AST_ScriptSerializerReload,
		.Gpu     = NULL,
		.End     = NULL,
		.Dispose = NULL,
	};

	return script_serializer;
}
