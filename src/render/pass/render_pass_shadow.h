#ifndef RENDER_PASS_SHADOW_H
#define RENDER_PASS_SHADOW_H

#define R_SHADOW_MAP_RESOLUTION 1024

typedef struct R_ShadowMappingPassData R_ShadowMappingPassData;
struct R_ShadowMappingPassData
{
	const R_FrameParams *frame_params;
	u32 caster_index;
	G_ResourceKey caster_table_buffer;
	R_DrawStream draw_stream;
};

internal R_PASS_RECORD_DEF(R_ShadowMappingPassFn);

typedef struct R_ShadowState R_ShadowState;
struct R_ShadowState
{
	G_ResourceKey shadow_cubemaps[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];
	G_ResourceKey shadow_cubemap_views[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];

	G_ResourceKey caster_table_buffer;
	u32 caster_count;
};

internal void R_ShadowsInit(R_ShadowState *st);
internal void R_ShadowsDestroy(R_ShadowState *st);

internal void R_ShadowsUploadGPU(R_ShadowState *st, const R_FrameParams *frame_params);

internal void R_ShadowsRender(R_ShadowState *st,
							  R_Graph *graph,
							  const R_FrameParams *frame_params,
							  R_Blackboard *bb);

#endif // RENDER_PASS_SHADOW_H
