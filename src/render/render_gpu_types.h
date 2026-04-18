#ifndef RENDER_GPU_TYPES_H
#define RENDER_GPU_TYPES_H

// TODO: Make gpu_types one file between both the SLANG and C++ codebases.

/*
#ifdef __cplusplus
#  define SLANG_PTR(T) u64
#else
#  define SLANG_PTR(T) T *
#endif
*/

typedef struct R_GPU_FrameData R_GPU_FrameData;
struct R_GPU_FrameData
{
	m4 view;
	m4 proj;
	m4 view_proj;
	m4 view_proj_no_translation;
	m4 inv_view;
	m4 inv_proj;
	v3 camera_position;
	v2 window_resolution;
	f32 time;
};

typedef struct R_GPU_ModelVertex R_GPU_ModelVertex;
struct R_GPU_ModelVertex
{
	AST_ModelVertex vertex;
};

typedef struct R_GPU_ObjectData R_GPU_ObjectData;
struct R_GPU_ObjectData
{
	m4 model_matrix;
	m4 normal_matrix;
	v4 sphere_bounds;
	u32 material_index;
	u32 page_index;
	u32 mesh_index;
};

typedef struct R_GPU_RenderMesh R_GPU_RenderMesh;
struct R_GPU_RenderMesh
{
	u32 index_count;
	u32 first_index;
	u64 vertex_buffer;
};

typedef struct R_GPU_Material R_GPU_Material;
struct R_GPU_Material
{
	u32 albedo_texture;
	u32 normal_texture;
	u32 emissive_texture;
	u32 metallic_roughness_texture;
	u32 ambient_texture;

	v4  albedo_factor;
	f32 metallic_factor;
	f32 roughness_factor;
	f32 emissive_factor;

	u32 double_sided;
};

typedef struct R_GPU_Light R_GPU_Light;
struct R_GPU_Light
{
	m4 transform;
	v3 position;
	f32 radius;
	v3 colour;
	f32 intensity;
	v3 attenuation;
	i32 shadow_slot_index; // -1 to disable shadows.
};

typedef struct R_GPU_ShadowCaster R_GPU_ShadowCaster;
struct R_GPU_ShadowCaster
{
	m4 face_matrices[6];
	v3 position;
	f32 near_plane;
	f32 far_plane;
	u32 shadow_map;
};

typedef struct R_GPU_IndirectDraw R_GPU_IndirectDraw;
struct R_GPU_IndirectDraw
{
	VkDrawIndexedIndirectCommand vk_command;
};

typedef struct R_GPU_PagePointers R_GPU_PagePointers;
struct R_GPU_PagePointers
{
	u64 vertex_buffer;
};

#endif // RENDER_GPU_TYPES_H
