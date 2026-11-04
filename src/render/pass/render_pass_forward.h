#ifndef RENDER_PASS_FORWARD_H
#define RENDER_PASS_FORWARD_H

typedef enum R_ForwardMode
{
	R_ForwardMode_Opaque,
	R_ForwardMode_Translucent,
	R_ForwardMode_COUNT
}
R_ForwardMode;

typedef struct R_ForwardPassData R_ForwardPassData;
struct R_ForwardPassData
{
	const R_FrameParams *frame_params;

	R_ForwardMode mode;

	u64 shadow_caster_table;

	u64 irradiance_sh_buffer_address;
	u64 irradiance_grid_info_buffer_address;

	R_GraphTexHandle irradiance_fb_handle;
	R_GraphTexHandle prefilter_handle;
	R_GraphTexHandle brdf_handle;

	R_DrawStream opaque_draw_stream;
};

internal R_PASS_RECORD_DEF(R_ForwardPassFn);

internal R_ForwardPassData *R_ForwardPassDataInit(R_Graph *graph, R_Pass *pass,
												  const R_FrameParams *frame_params,
												  const R_Blackboard *bb,
												  R_ForwardMode mode);

internal void R_ForwardRender(R_Graph *graph,
							  const R_FrameParams *frame_params,
							  R_Blackboard *bb,
							  const R_DrawStream *draw_stream);

internal void R_ForwardRenderTranslucent(R_Graph *graph,
										 const R_FrameParams *frame_params,
										 R_Blackboard *bb);

#endif // RENDER_PASS_FORWARD_H
