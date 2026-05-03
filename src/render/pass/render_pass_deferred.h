#ifndef RENDER_PASS_DEFERRED_H
#define RENDER_PASS_DEFERRED_H

typedef struct R_DeferredGeometryPassData R_DeferredGeometryPassData;
struct R_DeferredGeometryPassData
{
	GFX_ShaderKey shader;

	GFX_BufferKey frame_data_buffer;
	GFX_SamplerKey sampler;

	u64 object_buffer_address;

	R_DrawStream draw_stream;
};

R_PASS_RECORD_DEF(R_DeferredGeometryPassFn);


typedef struct R_DeferredLightingPassData R_DeferredLightingPassData;
struct R_DeferredLightingPassData
{
	GFX_ShaderKey ambient_shader;
	GFX_ShaderKey direct_shader;

	GFX_BufferKey frame_data_buffer;
	GFX_SamplerKey linear_sampler;

	u64 light_buffer_address;
	GFX_BufferKey shadow_caster_table;

	R_BB_GBufferData gbuffer;

	R_GraphTexHandle irradiance_fb_handle;
	R_GraphTexHandle prefilter_handle;
	R_GraphTexHandle brdf_handle;

	R_Mesh *light_sphere_mesh;

	u64 irradiance_sh_buffer_address;
	u64 irradiance_grid_info_buffer_address;
};

R_PASS_RECORD_DEF(R_DeferredLightingPassFn);

typedef struct R_DeferredRenderer R_DeferredRenderer;
struct R_DeferredRenderer
{
	GFX_Device *device;
	AST_Assets *assets;

	R_Mesh light_sphere_mesh;

	AST_Handle model_shader;
	AST_Handle ambient_lighting_shader;
	AST_Handle direct_lighting_point_shader;
};

internal void R_DeferredRendererInit               (R_DeferredRenderer *dr, GFX_Device *device, AST_Assets *assets);
internal void R_DeferredRendererDestroy            (R_DeferredRenderer *dr);

internal void R_DeferredRenderGeometry             (R_DeferredRenderer *dr,
													R_Graph *graph,
													R_Blackboard *bb,
													Arena *pass_arena,
													const R_SceneResources *scene_resources,
													GFX_BufferKey frame_data_buffer,
													GFX_SamplerKey linear_sampler,
													const R_DrawStream *draw_stream);

internal R_GraphTexHandle R_DeferredRenderLighting (R_DeferredRenderer *dr,
												    R_Graph *graph,
												    R_Blackboard *bb,
												    Arena *pass_arena,
												    const R_SceneResources *scene_resources,
												    GFX_BufferKey frame_data_buffer,
												    GFX_SamplerKey linear_sampler,
													const R_IrradianceVolume *irradiance_volume,
												    GFX_TextureKey irradiance_fallback,
												    GFX_TextureKey prefilter,
												    GFX_TextureKey brdf);

#endif // RENDER_PASS_DEFERRED_H
