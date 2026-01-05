#pragma once

#include <volk/volk.h>

#include "core/types.h"
#include "math/vec3.h"
#include "math/matrix.h"

namespace gfx
{

namespace gpu_types
{

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
};

struct GpuInstance {
	u32 object_id;
	u32 batch_id;
};

struct GpuIndirect {
	VkDrawIndexedIndirectCommand command;
};

struct GpuMaterial {
	u32 diffuse_texture;
	u32 normal_texture;
	u32 emissive_texture;
	u32 metallic_roughness_texture;
	u32 ambient_texture;
};

struct GpuLight {
	Vec3 position;
	Vec3 colour;
	Vec3 attenuation;
	float radius;
	Mat4 transform;
};

}

}
