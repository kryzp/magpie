#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#define R_FRAME_PARAMS_MAX_SHADOW_CASTERS 8

typedef struct R_ShadowCaster R_ShadowCaster;
struct R_ShadowCaster
{
	v3 position;
	f32 far;
	f32 near;
	f32 radius;
};

typedef struct R_FrameParams R_FrameParams;
struct R_FrameParams
{
	Arena *arena;

	u64 frame_number;

	f32 dt;
	f32 elapsed;

	R_Camera camera;

	G_BufferKey mesh_buffer;
	G_BufferKey material_buffer;

	G_Alloc frame_data;

	u32 page_count;
	G_Alloc page_table_buffer;
	G_BufferKey page_index_buffers[R_SCENE_MAX_GEOMETRY_PAGES];

	u32 object_count;
	G_Alloc object_buffer;

	u32 light_count;
	G_Alloc light_buffer;

	G_Alloc skinning_palette_buffer;

	//const R_IrradianceVolume *irradiance_volume;
	
	u32 shadow_caster_count;
	R_ShadowCaster shadow_casters[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];

	G_SamplerKey linear_sampler;
	G_SamplerKey nearest_sampler;
	G_TextureKey irradiance_fallback_cubemap;
	G_TextureKey prefilter_cubemap;
	G_TextureKey brdf;
};

typedef struct R_System R_System; // TODO TODO TODO: R_SYSTEM will not be taken in as a paremeter this is just a quick hack!!!!!

static void R_FrameParamsUploadPageTable(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
static void R_FrameParamsUploadSkinning(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
static void R_FrameParamsUploadObjects(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
static void R_FrameParamsUploadLights(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
static void R_FrameParamsUploadFrameData(G_RingBuffer *ring, R_FrameParams *out);

static R_FrameParams R_FrameParamsBuild(Arena *frame_arena,
										R_System *system,
										f32 dt, f32 elapsed,
										R_Scene *scene, const R_Camera *camera);

static void R_FrameParamsDrawIndirect(const R_FrameParams *frame_params,
									  G_CmdBuffer *cmd,
									  G_BufferKey indirect_buffer,
									  G_BufferKey count_buffer);

#endif // RENDER_FRAME_H
