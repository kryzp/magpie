#ifndef RENDER_BLACKBOARD_H
#define RENDER_BLACKBOARD_H

/*
 * Fixed Rendering Data.
 * Immutable.
 */
typedef struct R_Bulletin R_Bulletin;
struct R_Bulletin
{
	Arena *pass_arena;

	GFX_BufferKey frame_data_buffer;

	GFX_SamplerKey linear_sampler;
	GFX_SamplerKey nearest_sampler;

	const R_SceneFrameData *scene_resources;

	const R_IrradianceVolume *irradiance_volume;
	GFX_TextureKey irradiance_fallback_cubemap;
	GFX_TextureKey prefilter_cubemap;
	GFX_TextureKey brdf;
};

// ---

typedef struct R_BB_ShadowData R_BB_ShadowData;
struct R_BB_ShadowData
{
	u32 shadow_map_count;
	R_GraphTexHandle shadow_maps[R_SCENE_MAX_SHADOW_CASTERS];
	
	GFX_BufferKey shadow_caster_table;
};

/*
 * Transient Resources.
 * Mutable.
 */
typedef struct R_Blackboard R_Blackboard;
struct R_Blackboard
{
	R_GraphMsaaTexture lighting;
	R_GraphMsaaTexture depth;
	R_GraphTexHandle normals;
	R_BB_ShadowData shadow_data;
};

#endif // RENDER_BLACKBOARD_H
