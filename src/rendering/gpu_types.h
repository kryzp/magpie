#ifndef GFX_GPU_TYPES_H
#define GFX_GPU_TYPES_H

#include "core/core_math.h"

#include <volk/volk.h>

struct gfx_gpu_model_vertex {
	v3 position;
	v2 texcoord;
	v3 colour;
	v3 normal;
	v3 tangent;
	v3 bitangent;
};

struct gfx_gpu_frame_data {
	m4 view;
	m4 projection;
	m4 view_projection;
	m4 view_projection_no_translation;
	m4 inv_view;
	m4 inv_projection;
	v3 camera_position;
	v2 window_resolution;
	float time;
};

struct gfx_gpu_object_data {
	m4 model_matrix;
	m4 normal_matrix;
};

struct gfx_gpu_light {
	v4 position;    // xyz: position,    w: n/a
	v4 colour;      // xyz: colour,      w: intensity
	v4 attenuation; // xyz: attenuation, w: radius
	m4 transform;
};

struct gfx_gpu_material {
	u32 diffuse_texture;
	u32 normal_texture;
	u32 emissive_texture;
	u32 metallic_roughness_texture;
	u32 ambient_texture;
};

struct gfx_gpu_instance {
	u32 object_id;
	u32 batch_id;
};

struct gfx_gpu_indirect {
	VkDrawIndexedIndirectCommand command;
};

#endif // GFX_GPU_TYPES_H
