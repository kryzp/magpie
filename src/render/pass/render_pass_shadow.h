#ifndef RENDER_PASS_SHADOW_H
#define RENDER_PASS_SHADOW_H

#define R_SHADOW_MAP_RESOLUTION 2048

typedef struct R_ShadowMappingPassData R_ShadowMappingPassData;
struct R_ShadowMappingPassData
{
	G_ShaderKey shader;
	u32 caster_index;
	G_BufferKey caster_table_buffer;
	R_DrawStream draw_stream;
	const R_FrameParams *frame_params;
};

static R_PASS_RECORD_DEF(R_ShadowMappingPassFn);

typedef struct R_ShadowState R_ShadowState;
struct R_ShadowState
{
	G_TextureKey shadow_cubemaps[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];
	G_TextureViewKey shadow_cubemap_views[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];

	G_BufferKey caster_table_buffer;
	u32 caster_count;
};

static void R_ShadowsInit(R_ShadowState *st);
static void R_ShadowsDestroy(R_ShadowState *st);

static void R_ShadowsUploadGPU(R_ShadowState *st, const R_FrameParams *frame_params);

static void R_ShadowsRender(R_ShadowState *st,
							R_Graph *graph,
							const R_FrameParams *frame_params,
							R_Blackboard *bb);

#endif // RENDER_PASS_SHADOW_H
