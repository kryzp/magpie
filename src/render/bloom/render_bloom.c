
static void R_BloomRendererInit(R_BloomRenderer *renderer, A_Assets *assets)
{
	renderer->assets = assets;

	renderer->upsample_shader_handle   = A_Require(assets, String8Lit("assets://shaders/passes/upsample_bloom.slang"),   A_Type_Shader);
	renderer->downsample_shader_handle = A_Require(assets, String8Lit("assets://shaders/passes/downsample_bloom.slang"), A_Type_Shader);
}

static void R_BloomRendererDestroy(R_BloomRenderer *renderer)
{
}

static void R_BloomRender(R_BloomRenderer *renderer, R_Graph *graph)
{
}
