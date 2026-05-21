
typedef struct AST_ScriptLoadData AST_ScriptLoadData;
struct AST_ScriptLoadData
{
	u32 asdf;
};

internal AST_SerializerPipelineData
AST_ScriptSerializerCpu(const AST_Context *ctx, Arena *load_arena)
{
	DebugLogB(ctx->log_channel, "CPU NOT IMPLEMENTED");

	AST_SerializerPipelineData result = {0};

	return result;
}

internal void
AST_ScriptSerializerAlloc(const AST_Context *ctx,
						 AST_SerializerPipelineData *data,
						 AST_Asset *out,
						 Arena *arena)
{
	DebugLogB(ctx->log_channel, "ALLOC NOT IMPLEMENTED");
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
