#ifndef RENDER_PASS_FORWARD_H
#define RENDER_PASS_FORWARD_H

typedef struct R_ForwardPassData R_ForwardPassData;
struct R_ForwardPassData
{
	G_ShaderKey shader;

	const R_FrameParams *frame_params;

	G_BufferKey shadow_caster_table;

	u64 irradiance_sh_buffer_address;
	u64 irradiance_grid_info_buffer_address;

	R_GraphTexHandle irradiance_fb_handle;
	R_GraphTexHandle prefilter_handle;
	R_GraphTexHandle brdf_handle;

	R_DrawStream draw_stream;
};

static R_PASS_RECORD_DEF(R_ForwardPassFn);

static void R_ForwardRender(R_Graph *graph,
							const R_FrameParams *frame_params,
							R_Blackboard *bb,
							const R_DrawStream *draw_stream);

#endif // RENDER_PASS_FORWARD_H
