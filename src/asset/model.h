#ifndef ASSET_MODEL_H
#define ASSET_MODEL_H

#include "core/vec2.h"
#include "core/vec3.h"
#include "core/mat4.h"
#include "graphics/mesh.h"
#include "graphics/resource_key.h"

namespace ast {

// gpu-facing
struct ModelVertex {
	Vec3 position;
	Vec2 texcoord;
	Vec3 colour;
	Vec3 normal;
	Vec3 tangent;
	Vec3 bitangent;
};

// gpu-facing
struct ModelSkinVertex {
	unsigned joints[4];
	float weights[4];
};

struct SubModel {
	// = identity for skinned models!!
	Mat4 transform = Mat4::Identity();

	Vec3 bounds_min;
	Vec3 bounds_max;

	ModelMaterial material;

	Vector<ModelVertex> vertices;
	u64 vertex_stride = sizeof(ModelVertex);

	Vector<u32> indices;
	gfx::IndexType index_type = gfx::IndexType_U32;

	bool is_skinned = false;
	int skin_index = -1;          // -1 for invalid
	gfx::ResourceKey skin_buffer; // todo: get rid of this and export skin buffer size, this is stupid and dumb amnd hacky to be allocating here
};

}

#endif // ASSET_MODEL_H
