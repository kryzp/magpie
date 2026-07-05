#ifndef RENDER_PASS_SHADOW_H
#define RENDER_PASS_SHADOW_H

#define R_SHADOW_MAP_RESOLUTION 2048

typedef struct R_ShadowMappingPassData R_ShadowMappingPassData;
struct R_ShadowMappingPassData
{
	G_ShaderKey shader;

	u32 caster_index;

	u64 object_buffer_address;
	G_BufferKey caster_table_buffer;

	R_DrawStream draw_stream;
};

static R_PASS_RECORD_DEF(R_ShadowMappingPassFn);

typedef struct R_ShadowRenderer R_ShadowRenderer;
struct R_ShadowRenderer
{
	G_Device *device;
	A_Assets *assets;

	G_TextureKey shadow_cubemaps[R_SCENE_GRAPH_MAX_SHADOW_CASTERS];
	G_TextureViewKey shadow_cubemap_views[R_SCENE_GRAPH_MAX_SHADOW_CASTERS];

	G_BufferKey caster_table_buffer;
	u32 caster_count;

	A_Handle depth_shader;
};

/*
typedef struct R_ShadowState R_ShadowState;
struct R_ShadowState
{
	G_TextureKey shadow_cubemaps[R_SCENE_GRAPH_MAX_SHADOW_CASTERS];
	G_TextureViewKey shadow_cubemap_views[R_SCENE_GRAPH_MAX_SHADOW_CASTERS];

	G_BufferKey caster_table_buffer;
	u32 caster_count;
};
*/

static void R_ShadowRendererInit(R_ShadowRenderer *sr, G_Device *device, A_Assets *assets);
static void R_ShadowRendererDestroy(R_ShadowRenderer *sr);

static void R_ShadowRendererUploadGPU(R_ShadowRenderer *sr, const R_Bulletin *bt);

static void R_ShadowRendererRender(R_ShadowRenderer *sr,
									 R_Graph *graph,
									 const R_Bulletin *bt,
									 R_Blackboard *bb,
									 R_Culling *culling);

#endif // RENDER_PASS_SHADOW_H
