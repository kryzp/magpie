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

	G_ResourceKey mesh_buffer;
	G_ResourceKey material_buffer;

	G_Alloc frame_data;

	u32 page_count;
	G_Alloc page_table_buffer;
	G_ResourceKey page_index_buffers[R_SCENE_MAX_GEOMETRY_PAGES];
	G_IndexType page_index_types[R_SCENE_MAX_GEOMETRY_PAGES];

	u32 object_count;
	G_Alloc object_buffer;

	u32 light_count;
	G_Alloc light_buffer;

	G_Alloc skinning_palette_buffer;

	//const R_IrradianceVolume *irradiance_volume;
	
	u32 shadow_caster_count;
	R_ShadowCaster shadow_casters[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];

	G_ResourceKey brdf_lut;
	G_ResourceKey irradiance_fallback_cubemap;
	G_ResourceKey prefilter_cubemap;

	const R_Mesh *skybox_mesh;

	G_ResourceKey cubemap_capture_transform_buffer;

	G_ResourceKey linear_sampler;
	G_ResourceKey nearest_sampler;
	
	G_ResourceKey debug_line_shader;
	G_ResourceKey forward_shader;
	G_ResourceKey shadow_shader;
	G_ResourceKey cull_frustum_shader;
	G_ResourceKey cull_sphere_shader;
	G_ResourceKey skybox_shader;
	G_ResourceKey tonemapping_shader;
	G_ResourceKey brdf_lut_generation_shader;
	G_ResourceKey hdr_to_cubemap_shader;
	G_ResourceKey irradiance_cubemap_gen_shader;
	G_ResourceKey prefilter_cubemap_gen_shader;
};

typedef struct R_System R_System; // TODO TODO TODO: R_SYSTEM will not be taken in as a paremeter this is just a quick hack!!!!!

internal void R_FrameParamsUploadPageTable(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
internal void R_FrameParamsUploadSkinning(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
internal void R_FrameParamsUploadObjects(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
internal void R_FrameParamsUploadLights(R_Scene *scene, G_RingBuffer *ring, R_FrameParams *out);
internal void R_FrameParamsUploadFrameData(G_RingBuffer *ring, R_FrameParams *out);
internal void R_FrameParamsResolveShaders(R_System *system, R_FrameParams *out);

internal R_FrameParams R_FrameParamsBuild(Arena *frame_arena,
										R_System *system,
										u32 frame_number, f32 dt, f32 elapsed,
										R_Scene *scene, const R_Camera *camera);

internal void R_FrameParamsDrawIndirect(const R_FrameParams *frame_params,
									  G_CmdBuffer *cmd,
									  G_ResourceKey indirect_buffer,
									  G_ResourceKey count_buffer);

#endif // RENDER_FRAME_H
