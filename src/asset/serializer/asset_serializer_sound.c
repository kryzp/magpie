
typedef struct AST_SoundLoadData AST_SoundLoadData;
struct AST_SoundLoadData
{
};

internal AST_LoadData
AST_SoundSerializerCpu(const AST_Context *ctx)
{
}

internal void
AST_SoundSerializerAlloc(const AST_Context *ctx,
						 AST_LoadData *data,
						 GFX_Device *device,
						 AST_Asset *out)
{
}

internal void
AST_SoundSerializerReload(const AST_Context *ctx,
						  AST_LoadData *data,
						  GFX_Device *device,
						  AST_Asset *existing)
{
	// TODO
}

internal void
AST_SoundSerializerDispose(AST_Asset *asset, GFX_Device *device)
{
}

internal AST_Serializer
AST_GetSoundSerializer(void)
{
	static AST_Serializer sound_serializer = {
		.Cpu     = AST_SoundSerializerCpu,
		.Alloc   = AST_SoundSerializerAlloc,
		.Reload  = AST_SoundSerializerReload,
		.Gpu     = NULL,
		.End     = NULL,
		.Dispose = AST_SoundSerializerDispose,
	};

	return sound_serializer;
}
