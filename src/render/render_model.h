#ifndef RENDER_MODEL_H
#define RENDER_MODEL_H

typedef u32 R_MeshIndex;

typedef struct R_Mesh R_Mesh;
struct R_Mesh
{
	u64 vertex_stride;

	u32 vertex_count;
	u32 index_count;

	GFX_BufferKey vertex_buffer;
	GFX_BufferKey index_buffer;
};

internal void R_MeshAlloc(R_Mesh *mesh, GFX_Device *device,
						  u64 vertex_stride,
						  u32 vertex_count, u32 index_count);

internal void R_MeshDestroy(const R_Mesh *mesh, GFX_Device *device);

internal void R_MeshWriteToStage(const R_Mesh *mesh,
								 GFX_Buffer *stage, u64 stage_base,
								 void *vertices, R_MeshIndex *indices);

internal u64 R_MeshUpload(const R_Mesh *mesh,
						  const GFX_Device *device,
						  const GFX_CmdBuffer *cmd,
						  GFX_Buffer *stage, u64 stage_base);

internal inline u64
R_MeshVertexBufferSize(const R_Mesh *mesh)
{
	return mesh->vertex_count * mesh->vertex_stride;
}

internal inline u64
R_MeshIndexBufferSize(const R_Mesh *mesh)
{
	return mesh->index_count * sizeof(R_MeshIndex);
}

typedef struct R_Material R_Material;
struct R_Material
{
	AST_Handle albedo;
	AST_Handle normal;
	AST_Handle emissive;
	AST_Handle metallic_roughness;
	AST_Handle ambient;
};

typedef struct R_SubModel R_SubModel;
struct R_SubModel
{
	R_SubModel *next;
	
	m4 transform;
	v4 sphere;
	
	R_Mesh mesh;
	R_Material material;
};

typedef struct R_Model R_Model;
struct R_Model
{
	R_SubModel *sub_model_head;
};

#endif // RENDER_MODEL_H
