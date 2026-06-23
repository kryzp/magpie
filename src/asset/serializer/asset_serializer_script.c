
typedef struct A_ScriptLoadData A_ScriptLoadData;
struct A_ScriptLoadData
{
	S_Ref ref;
};

static A_SerializerPipelineData A_ScriptSerializerCpu(const A_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = A_ContextSystemFilePath(ctx, scratch.arena);

	A_ScriptLoadData *script = ArenaPushArray(load_scope, A_ScriptLoadData, 1);
	script->ref = S_RefNull();
	
	IO_ByteSpan source_bytes = IO_ReadEntireFile(scratch.arena, file_path);
	S_Ref chunk_ref = S_Compile(ctx->assets->scripting_system, source_bytes, file_path);

	if (!S_RefIsNull(chunk_ref))
	{
		script->ref = S_ExecuteModule(ctx->assets->scripting_system, chunk_ref);
		S_Release(ctx->assets->scripting_system, chunk_ref);
	}
	
	A_SerializerPipelineData result = {0};
	result.data = script;
	result.stage_size = 0;
	result.failed = false;
	result.dependency_count = 0;

	ScratchRelease(&scratch);
	
	return result;
}

static void A_ScriptSerializerAlloc(const A_Context *ctx,
						 A_SerializerPipelineData *data,
						 A_Asset *out,
						 Arena *arena)
{
	A_ScriptLoadData *script = data->data;

	out->script.ref = script->ref;
}

static void A_ScriptSerializerReload(const A_Context *ctx,
						  A_SerializerPipelineData *data,
						  A_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

static A_Serializer A_GetScriptSerializer(void)
{
	static A_Serializer script_serializer = {
		.Cpu     = A_ScriptSerializerCpu,
		.Alloc   = A_ScriptSerializerAlloc,
		.Reload  = A_ScriptSerializerReload,
		.Gpu     = NULL,
		.End     = NULL,
		.Dispose = NULL,
	};

	return script_serializer;
}
