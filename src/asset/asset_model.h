#ifndef ASSET_MODEL_H
#define ASSET_MODEL_H

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

	void *vertices;
	u32 vertex_count;
	u64 vertex_stride;
	
	void *indices;
	u32 index_count;
	G_IndexType index_type;

	b32 is_skinned;
	i32 skin_index; // -1 for invalid
	G_ResourceKey skin_buffer; // todo: get rid of this and export skin buffer size, this is stupid and dumb amnd hacky to be allocating here
	//u64 skin_buffer_size;
};

#endif // ASSET_MODEL_H
