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
	u32 frame_number;
	f32 delta_time;
	f32 time;
};

typedef struct R_GPU_ModelVertex R_GPU_ModelVertex;
struct R_GPU_ModelVertex
{
	A_ModelVertex vertex;
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
	
	u32 skinning_joint_count;
	u64 skinning_palette_buffer;
};

typedef struct R_GPU_RenderMesh R_GPU_RenderMesh;
struct R_GPU_RenderMesh
{
	u32 index_count;
	u32 first_index;
	u64 vertex_buffer;
	u64 skin_buffer;
};

typedef struct R_GPU_Material R_GPU_Material;
struct R_GPU_Material
{
	u32 albedo_texture;
	u32 normal_texture;
	u32 metallic_roughness_texture;
	u32 emissive_texture;
	u32 occlusion_texture;
	
	v4  albedo_factor;
	f32 normal_scale;
	f32 metallic_factor;
	f32 roughness_factor;
	v3  emissive_factor;
	f32 emissive_intensity;
	f32 occlusion_intensity;

	// ---

	f32 ior;
	
	// ---
	
	u32 transmission_texture;
	u32 thickness_texture;

	f32 transmission_factor;
	f32 thickness_factor;

	v3  attenuation_colour;
	f32 attenuation_distance;

	// ---
	
	u32 specular_texture;
	u32 specular_colour_texture;

	f32 specular_factor;
	v3  specular_colour_factor;

	// ---
	
	u32 clearcoat_texture;
	u32 clearcoat_roughness_texture;

	f32 clearcoat_factor;
	f32 clearcoat_roughness_factor;

	// ---
	
	u32 sheen_colour_texture;
	u32 sheen_roughness_texture;

	v3  sheen_colour_factor;
	f32 sheen_roughness_factor;
	
	// ---
	
	u32 iridescence_texture;
	u32 iridescence_thickness_texture;

	f32 iridescence_factor;
	f32 iridescence_ior;
	f32 iridescence_thickness_min_nanometers;
	f32 iridescence_thickness_max_nanometers;

	// ---
	
	u32 double_sided;
	u32 unlit;
	f32 alpha_cutoff;
	u32 alpha_mode;
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
