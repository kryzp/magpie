
internal const GFX_ShaderCompilerAPI *
GFX_GetShaderCompilerAPI(void)
{
	static GFX_ShaderCompilerAPI api = {0};

	return &api;
}
