
typedef struct AST_ModelLoadData AST_ModelLoadData;
struct AST_ModelLoadData
{
};

internal AST_LoadData
AST_ModelSerializerCpu(const AST_Context *ctx)
{
}

internal void
AST_ModelSerializerAlloc(const AST_Context *ctx,
						   AST_LoadData *data,
						   GFX_Device *device,
						   AST_Asset *out)
{
}

internal void
AST_ModelSerializerReload(const AST_Context *ctx,
							AST_LoadData *data,
							GFX_Device *device,
							AST_Asset *existing)
{
	// TODO
}

internal void
AST_ModelSerializerGpu(const AST_Context *ctx,
						 AST_LoadData *data,
						 AST_Asset *asset,
						 GFX_Device *device,
						 GFX_CmdBuffer *cmd,
						 GFX_Buffer *stage, u64 stage_base)
{
}

internal void
AST_ModelSerializerEnd(AST_LoadData *data)
{
}

internal void
AST_ModelSerializerDispose(AST_Asset *asset, GFX_Device *device)
{
}

internal AST_Serializer
AST_GetModelSerializer(void)
{
	static AST_Serializer model_serializer = {
		.Cpu     = AST_ModelSerializerCpu,
		.Alloc   = AST_ModelSerializerAlloc,
		.Reload  = AST_ModelSerializerReload,
		.Gpu     = AST_ModelSerializerGpu,
		.End     = AST_ModelSerializerEnd,
		.Dispose = AST_ModelSerializerDispose,
	};

	return model_serializer;
}
