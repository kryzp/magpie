#ifndef RENDER_BLACKBOARD_H
#define RENDER_BLACKBOARD_H

// Realistically should just be one struct
// but I've split up "global" render data
// (i.e: general data required by all render
// systems) into two structs - an "immutable"
// one (bulletin) and a "mutable" one (blackboard)
// immutable - for constant data
// mutable - typically attachments that get written/read from

typedef struct R_Bulletin R_Bulletin;
struct R_Bulletin
{
	Arena *pass_arena;

	G_BufferKey frame_data_buffer;

	G_SamplerKey linear_sampler;
	G_SamplerKey nearest_sampler;

	const R_SceneFrameData *scene_resources;

	const R_IrradianceVolume *irradiance_volume;
	G_TextureKey irradiance_fallback_cubemap;
	G_TextureKey prefilter_cubemap;
	G_TextureKey brdf;
};

typedef struct R_BB_ShadowData R_BB_ShadowData;
struct R_BB_ShadowData
{
	u32 shadow_map_count;
	R_GraphTexHandle shadow_maps[R_SCENE_MAX_SHADOW_CASTERS];
	
	G_BufferKey shadow_caster_table;
};

typedef struct R_Blackboard R_Blackboard;
struct R_Blackboard
{
	R_GraphMsaaTexture lighting;
	R_GraphMsaaTexture depth;
	R_GraphTexHandle normals;
	R_BB_ShadowData shadow_data;
};

#endif // RENDER_BLACKBOARD_H
