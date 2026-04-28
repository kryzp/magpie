#ifndef RENDER_PASS_SHADOW_H
#define RENDER_PASS_SHADOW_H

#define R_SHADOW_MAP_RESOLUTION 2048


/* ==================================================
   SHADOW MAPPING PASS
   ================================================== */

typedef struct R_ShadowMappingPassData R_ShadowMappingPassData;
struct R_ShadowMappingPassData
{
	GFX_ShaderKey shader;

	u32 caster_index;

	u64 object_buffer_address;
	u64 mesh_buffer_address;
	GFX_BufferKey caster_table_buffer;

	R_DrawStream draw_stream;
};

R_PASS_RECORD_DEF(R_ShadowMappingPassFn);


/* ==================================================
   SHADOW RENDERER
   ================================================== */

typedef struct R_ShadowRenderer R_ShadowRenderer;
struct R_ShadowRenderer
{
	GFX_Device *device;
	AST_Assets *assets;

	GFX_TextureKey     shadow_cubemaps      [R_SCENE_MAX_SHADOW_CASTERS];
	GFX_TextureViewKey shadow_cubemap_views [R_SCENE_MAX_SHADOW_CASTERS];

	GFX_BufferKey caster_table_buffer;

	AST_Handle depth_shader;
};

internal void R_ShadowRendererInit    (R_ShadowRenderer *sr, GFX_Device *device, AST_Assets *assets);
internal void R_ShadowRendererDestroy (R_ShadowRenderer *sr);

internal void R_ShadowRendererRender  (R_ShadowRenderer *sr,
									   R_Graph *graph,
									   R_Blackboard *bb,
									   Arena *pass_arena,
									   R_Culling *culling,
									   const R_Scene *scene,
									   const R_SceneResources *scene_resources);

#endif // RENDER_PASS_SHADOW_H
