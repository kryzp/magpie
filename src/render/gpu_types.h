#ifndef RENDER_GPU_TYPES_H
#define RENDER_GPU_TYPES_H

#include "core/types.h"
#include "core/maths.h"
#include "asset/types.h"

#include <volk/volk.h>

// These mirror the structs in the Slang shaders, so field order and widths matter.
// TODO: Make gpu_types one file between both the SLANG and C++ codebases.

namespace rnd {
struct GPU_FrameData {
	Mat4 view;
	Mat4 proj;
	Mat4 view_proj;
	Mat4 view_proj_no_translation;
	Mat4 inv_view;
	Mat4 inv_proj;
	Vec3 camera_position;
	Vec2 window_resolution;
	u32 frame_number;
	float delta_time;
	float time;
};

struct GPU_ModelVertex {
	ast::ModelVertex vertex;
};

struct GPU_ObjectData {
	Mat4 model_matrix;
	Mat4 normal_matrix;

	Vec4 local_sphere_bounds;

	u32 material_index;
	u32 page_index;
	u32 mesh_index;

	u32 skinning_joint_count;
	u64 skinning_palette_buffer;
};

struct GPU_RenderMesh {
	u32 index_count;
	u32 first_index;
	u64 vertex_buffer;
	u64 skin_buffer;
};

struct GPU_Material {
	u32 albedo_texture;
	u32 normal_texture;
	u32 metallic_roughness_texture;
	u32 emissive_texture;
	u32 occlusion_texture;

	Vec4 albedo_factor;
	float normal_scale;
	float metallic_factor;
	float roughness_factor;
	Vec3 emissive_factor;
	float emissive_intensity;
	float occlusion_intensity;

	// ---

	float ior;

	// ---

	u32 transmission_texture;
	u32 thickness_texture;

	float transmission_factor;
	float thickness_factor;

	Vec3 attenuation_colour;
	float attenuation_distance;

	// ---

	u32 specular_texture;
	u32 specular_colour_texture;

	float specular_factor;
	Vec3 specular_colour_factor;

	// ---

	u32 clearcoat_texture;
	u32 clearcoat_roughness_texture;

	float clearcoat_factor;
	float clearcoat_roughness_factor;

	// ---

	u32 sheen_colour_texture;
	u32 sheen_roughness_texture;

	Vec3 sheen_colour_factor;
	float sheen_roughness_factor;

	// ---

	u32 iridescence_texture;
	u32 iridescence_thickness_texture;

	float iridescence_factor;
	float iridescence_ior;
	float iridescence_thickness_min_nanometers;
	float iridescence_thickness_max_nanometers;

	// ---

	u32 double_sided;
	u32 unlit;

	float alpha_cutoff;
	u32 alpha_mode;

	u32 gloss_in_specular_alpha;
};

struct GPU_Light {
	Mat4 transform;
	Vec3 position;
	float radius;
	Vec3 colour;
	float intensity;
	Vec3 attenuation;
	i32 shadow_slot_index; // -1 to disable shadows.
};

struct GPU_ShadowCaster {
	Mat4 face_matrices[6]; // reversed-z
	Vec3 position;
	float near_plane;
	float far_plane;
	u32 shadow_map;
	float texel_world_scale; // world-space texel size at distance 1 from the light (2 / resolution)
	float cull_radius;
};

struct GPU_IndirectDraw {
	VkDrawIndexedIndirectCommand vk_command;
};

struct GPU_DebugObjectDraw {
	Mat4 transform;
	Vec4 colour;
	float thickness;
};

struct GPU_PagePointers {
	u64 vertex_buffer;
};

static_assert(sizeof(Vec2) == 8,  "GPU layout expects tightly packed Vec2.");
static_assert(sizeof(Vec3) == 12, "GPU layout expects tightly packed Vec3.");
static_assert(sizeof(Vec4) == 16, "GPU layout expects tightly packed Vec4.");
static_assert(sizeof(Mat4) == 64, "GPU layout expects tightly packed Mat4.");

}

#endif // RENDER_GPU_TYPES_H
