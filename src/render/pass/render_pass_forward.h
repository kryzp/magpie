#ifndef RENDER_PASS_FORWARD_H
#define RENDER_PASS_FORWARD_H

typedef struct R_ForwardPassData R_ForwardPassData;
struct R_ForwardPassData
{
	G_ShaderKey shader;

	G_BufferKey frame_data_buffer;
	G_BufferKey shadow_caster_table;
	
	G_SamplerKey linear_sampler;
	G_SamplerKey nearest_sampler;

	u64 object_buffer_address;
	u64 light_buffer_address;

	u64 irradiance_sh_buffer_address;
	u64 irradiance_grid_info_buffer_address;

	R_GraphTexHandle irradiance_fb_handle;
	R_GraphTexHandle prefilter_handle;
	R_GraphTexHandle brdf_handle;

	R_DrawStream draw_stream;
};

R_PASS_RECORD_DEF(R_ForwardPassFn);


typedef struct R_ForwardRenderer R_ForwardRenderer;
struct R_ForwardRenderer
{
	G_Device *device;
	A_Registry *assets;

	A_Handle shader;
};

internal void R_ForwardRendererInit    (R_ForwardRenderer *r, G_Device *device, A_Registry *assets);
internal void R_ForwardRendererDestroy (R_ForwardRenderer *r);

internal void R_ForwardRender          (R_ForwardRenderer *r,
										R_Graph *graph,
										const R_Bulletin *bt,
										R_Blackboard *bb,
										const R_DrawStream *draw_stream);

#endif // RENDER_PASS_FORWARD_H
