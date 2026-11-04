#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

/*
 * FRAME PACKET - built by the game, sent over to rendering system
 * FRAME PARAMS - built from the packet, contains all the GPU-ready data
 */

#define R_FRAME_SLOT_COUNT                   2
#define R_FRAME_PARAMS_MAX_SHADOW_CASTERS    8
#define R_FRAME_UPLOAD_BUDGET                Megabytes(128)
#define R_DEBUG_MAX_BATCHES                  64


typedef u32 R_ObjectFlags;
enum
{
	R_ObjectFlag_None        = 0,
	R_ObjectFlag_Translucent = 1 << 0,
};

typedef struct R_PacketObject R_PacketObject;
struct R_PacketObject
{
	m4 transform;
	m4 normal_matrix;
	v4 local_sphere_bounds;

	u32 page_index;
	u32 mesh_index;
	u32 material_index;

	// note to self: copied by value during collect so animation system can rewrite pallette
	u32 skinning_first_matrix;
	u32 skinning_joint_count;

	R_ObjectFlags flags;
};

typedef struct R_PacketGeometryPage R_PacketGeometryPage;
struct R_PacketGeometryPage
{
	u64 vertex_buffer_address;
	G_ResourceKey index_buffer;
	G_IndexType index_type;
};

typedef enum R_DebugDepthMode
{
	R_DebugDepthMode_Tested, // depth test
	R_DebugDepthMode_Overlay, // no depth test
	R_DebugDepthMode_COUNT
}
R_DebugDepthMode;

typedef struct R_DebugBatch R_DebugBatch;
struct R_DebugBatch
{
	u32 type; // R_DebugDrawType
	u32 start;
	u32 count;
};

typedef struct R_DebugPacket R_DebugPacket;
struct R_DebugPacket
{
	u32 instance_count[R_DebugDepthMode_COUNT];
	R_GPU_DebugObjectDraw *instances[R_DebugDepthMode_COUNT];

	u32 batch_count[R_DebugDepthMode_COUNT];
	R_DebugBatch batches[R_DebugDepthMode_COUNT][R_DEBUG_MAX_BATCHES];
};

typedef struct R_FramePacket R_FramePacket;
struct R_FramePacket
{
	Arena arena;

	u64 frame_number;
	f32 dt;
	f32 elapsed;

	u32 window_width;
	u32 window_height;

	R_Camera camera;

	u32 object_count;
	R_PacketObject *objects;

	u32 skinning_matrix_count;
	m4 *skinning_matrices;

	u32 light_count;
	R_Light *lights;

	u32 mesh_count;
	R_GPU_RenderMesh *meshes;

	u32 material_count;
	R_GPU_Material *materials;

	u32 page_count;
	R_PacketGeometryPage pages[R_SCENE_MAX_GEOMETRY_PAGES];

	R_DebugPacket debug;
};

internal void R_FramePacketBegin(R_FramePacket *packet, u64 frame_number, f32 dt, f32 elapsed);
internal void R_FramePacketCollectScene(R_FramePacket *packet, const R_Scene *scene, const R_Camera *camera, f32 interp_t);


typedef struct R_ShadowCaster R_ShadowCaster;
struct R_ShadowCaster
{
	v3 position;
	f32 far;
	f32 near;
	f32 radius;
};

typedef struct R_TranslucentDraw R_TranslucentDraw;
struct R_TranslucentDraw
{
	u32 object_index; // first instance
	u32 page_index;
	u32 index_count;
	u32 first_index;
};

typedef struct R_FrameParams R_FrameParams;
struct R_FrameParams
{
	Arena *arena;
	G_RingBuffer *upload;

	const R_FramePacket *packet;

	u64 frame_number;

	f32 dt;
	f32 elapsed;

	R_Camera camera;

	G_Alloc frame_data;

	G_Alloc mesh_table;
	G_Alloc material_table;

	u32 page_count;
	G_Alloc page_table_buffer;
	G_ResourceKey page_index_buffers[R_SCENE_MAX_GEOMETRY_PAGES];
	G_IndexType page_index_types[R_SCENE_MAX_GEOMETRY_PAGES];

	u32 object_count;
	G_Alloc object_buffer;

	u32 light_count;
	G_Alloc light_buffer;

	G_Alloc skinning_palette_buffer;

	u32 shadow_caster_count;
	R_ShadowCaster shadow_casters[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];
	G_Alloc shadow_caster_table;

	u32 translucent_count;
	R_TranslucentDraw *translucent_draws; // sorted BTF

	G_ResourceKey brdf_lut;
	G_ResourceKey irradiance_fallback_cubemap;
	G_ResourceKey prefilter_cubemap;

	const R_Mesh *skybox_mesh;

	G_ResourceKey cubemap_capture_transform_buffer;

	G_ResourceKey linear_sampler;
	G_ResourceKey nearest_sampler;

	// to prevent a shader hot-swap from changing mid-frame
	b32 shaders_ready;

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

typedef struct R_System R_System;

internal R_FrameParams R_FrameParamsBuild(Arena *arena,
										  R_System *system,
										  G_RingBuffer *upload,
										  const R_FramePacket *packet);

internal void R_FrameParamsDrawIndirect(const R_FrameParams *frame_params,
										G_CmdBuffer *cmd,
										G_ResourceKey indirect_buffer,
										G_ResourceKey count_buffer);

internal void R_FrameParamsDrawTranslucent(const R_FrameParams *frame_params, G_CmdBuffer *cmd);

#endif // RENDER_FRAME_H
