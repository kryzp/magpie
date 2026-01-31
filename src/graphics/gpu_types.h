#pragma once

#include <volk/volk.h>

#include "core/types.h"
#include "math/vec3.h"
#include "math/matrix.h"

// TODO: Make gpu_types one file between both the SLANG and C++ codebases.
/*
#ifdef __cplusplus
#  define SLANG_PTR(T) u64
#else
#  define SLANG_PTR(T) T *
#endif
*/

namespace gfx
{
	namespace gpu_types
	{
		struct GpuFrameData {
			Mat4 view;
			Mat4 projection;
			Mat4 view_projection;
			Mat4 view_projection_no_translation;
			Mat4 inv_view;
			Mat4 inv_projection;
			Vec3 camera_position;
			Vec2 window_resolution;
			float time;
		};

		struct GpuModelVertex {
			Vec3 position;
			Vec2 texcoord;
			Vec3 colour;
			Vec3 normal;
			Vec3 tangent;
			Vec3 bitangent;
		};

		struct GpuObjectData {
			Mat4 model_matrix;
			Mat4 normal_matrix;
			Vec4 sphere_bounds; // xyz = centre, w = radius
			u32 material_index;
			u32 page_index; // Which geometry page this object belongs to.
			u32 mesh_index;
		};

		struct GpuMesh {
			u32 index_count;
			u32 first_index;
			u64 vertex_buffer;
		};

		struct GpuMaterial {
			u32 albedo_texture;
			u32 normal_texture;
			u32 emissive_texture;
			u32 metallic_roughness_texture;
			u32 ambient_texture;
		};

		struct GpuLight {
			Vec3 position;
			Vec3 colour;
			float intensity;
			Vec3 attenuation;
			float radius;
			Mat4 transform;
		};

		struct GpuIndirectDraw {
			VkDrawIndexedIndirectCommand vk_command;
		};

		struct GpuPagePointers {
			u64 indirect_buffer;
			u64 count_buffer;
			u64 vertex_buffer;
		};
	}
}
