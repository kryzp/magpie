
typedef struct AST_ShaderLoadData AST_ShaderLoadData;
struct AST_ShaderLoadData
{
};

internal AST_LoadData
AST_ShaderSerializerCpu(const AST_Context *ctx)
{
}

internal void
AST_ShaderSerializerAlloc(const AST_Context *ctx,
						  AST_LoadData *data,
						  GFX_Device *device,
						  AST_Asset *out)
{
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
