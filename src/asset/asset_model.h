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

typedef struct AST_ModelMaterial AST_ModelMaterial;
struct AST_ModelMaterial
{
	AST_Handle albedo;
	AST_Handle normal;
	AST_Handle emissive;
	AST_Handle metallic_roughness;
	AST_Handle ambient;
};

typedef struct AST_ModelMesh AST_ModelMesh;
struct AST_ModelMesh
{
	m4 transform;

	v3 bounds_min;
	v3 bounds_max;

	u64 vertex_stride;
	u64 index_stride;

	u32 vertex_count;
	u32 index_count;

	GFX_BufferKey vertex_buffer;
	GFX_BufferKey index_buffer;

	AST_ModelMaterial material;
};

#endif // ASSET_MODEL_H
