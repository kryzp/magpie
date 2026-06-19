#ifndef ASSET_MODEL_H
#define ASSET_MODEL_H

typedef u32 A_ModelIndex;

typedef struct A_ModelVertex A_ModelVertex;
struct A_ModelVertex
{
	v3 position;
	v2 texcoord;
	v3 colour;
	v3 normal;
	v3 tangent;
	v3 bitangent;
};

typedef struct A_ModelSkinVertex A_ModelSkinVertex;
struct A_ModelSkinVertex
{
	u32 joints[4];
	f32 weights[4];
};

typedef struct A_SubModel A_SubModel;
struct A_SubModel
{
	// note: identity for skinned models!!
	m4 transform;

	v3 bounds_min;
	v3 bounds_max;

	A_ModelMaterial material;

	u64 vertex_stride;
	u64 index_stride;

	u32 vertex_count;
	u32 index_count;

	G_BufferKey vertex_buffer;
	G_BufferKey index_buffer;

	G_BufferKey skin_buffer;
	i32 skin_index;
};

#endif // ASSET_MODEL_H
