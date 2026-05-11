#ifndef ASSET_MODEL_H
#define ASSET_MODEL_H

typedef u32 AST_ModelIndex;

typedef struct AST_ModelVertex AST_ModelVertex;
struct AST_ModelVertex
{
	v3 position;
	v2 texcoord;
	v3 colour;
	v3 normal;
	v3 tangent;
	v3 bitangent;
};

typedef struct AST_ModelSkinVertex AST_ModelSkinVertex;
struct AST_ModelSkinVertex
{
	u32 joints[4];
	f32 weights[4];
};

typedef struct AST_SubModel AST_SubModel;
struct AST_SubModel
{
	// note: identity for skinned models!!
	m4 transform;

	v3 bounds_min;
	v3 bounds_max;

	AST_ModelMaterial material;

	u64 vertex_stride;
	u64 index_stride;

	u32 vertex_count;
	u32 index_count;

	GFX_BufferKey vertex_buffer;
	GFX_BufferKey index_buffer;

	GFX_BufferKey skin_buffer;
	i32 skin_index;
};

#endif // ASSET_MODEL_H
