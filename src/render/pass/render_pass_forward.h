#ifndef RENDER_PASS_FORWARD_H
#define RENDER_PASS_FORWARD_H

typedef struct R_ForwardPassData R_ForwardPassData;
struct R_ForwardPassData
{
	GFX_ShaderKey shader;

	GFX_BufferKey frame_data_buffer;
	GFX_BufferKey shadow_caster_table;
	
	GFX_SamplerKey linear_sampler;
	GFX_SamplerKey nearest_sampler;

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
	GFX_Device *device;
	AST_Assets *assets;

	AST_Handle shader;
};

internal void R_ForwardRendererInit       (R_ForwardRenderer *r, GFX_Device *device, AST_Assets *assets);
internal void R_ForwardRendererDestroy    (R_ForwardRenderer *r);

internal R_GraphTexHandle R_ForwardRender (R_ForwardRenderer *r,
										   R_Graph *graph,
										   R_Blackboard *bb,
										   Arena *pass_arena,
										   const R_SceneResources *scene_resources,
										   GFX_BufferKey frame_data_buffer,
										   GFX_SamplerKey linear_sampler,
										   GFX_SamplerKey nearest_sampler,
										   const R_IrradianceVolume *irradiance_volume,
										   GFX_TextureKey irradiance_fallback,
										   GFX_TextureKey prefilter,
										   GFX_TextureKey brdf,
										   const R_DrawStream *draw_stream);

#endif // RENDER_PASS_FORWARD_H
