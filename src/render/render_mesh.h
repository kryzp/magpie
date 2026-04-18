#ifndef RENDER_MESH_H
#define RENDER_MESH_H

typedef struct R_Mesh R_Mesh;
struct R_Mesh
{
	u64 vertex_stride;
	u64 index_stride;

	u32 vertex_count;
	u32 index_count;

	GFX_BufferKey vertex_buffer;
	GFX_BufferKey index_buffer;
};

internal void R_MeshAlloc(R_Mesh *mesh, GFX_Device *device,
						  u64 vertex_stride, u64 index_stride,
						  u32 vertex_count, u32 index_count);

internal void R_MeshDestroy(const R_Mesh *mesh, GFX_Device *device);

internal void R_MeshWriteToStage(const R_Mesh *mesh,
								 GFX_Buffer *stage, u64 stage_base,
								 void *vertices, void *indices);

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
	return mesh->index_count * mesh->index_stride;
}

#endif // RENDER_MESH_H
