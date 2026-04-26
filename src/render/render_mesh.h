#ifndef RENDER_MESH_H
#define RENDER_MESH_H

typedef struct R_Mesh R_Mesh;
struct R_Mesh
{
	u64 vertex_stride;
	VkIndexType index_type;

	u32 vertex_count;
	u32 index_count;

	GFX_BufferKey vertex_buffer;
	GFX_BufferKey index_buffer;
};

internal void R_MeshAlloc(R_Mesh *mesh, GFX_Device *device,
						  u64 vertex_stride, VkIndexType index_type,
						  u32 vertex_count, u32 index_count);

internal void R_MeshDestroy(const R_Mesh *mesh, GFX_Device *device);

internal void R_MeshWriteToStage(const R_Mesh *mesh, GFX_Device *device,
								 GFX_BufferKey stage, u64 stage_base,
								 void *vertices, void *indices);

internal u64 R_MeshUpload(const R_Mesh *mesh, const GFX_CmdBuffer *cmd,
						  GFX_BufferKey stage, u64 stage_base);

//internal u64 R_MeshVertexAddress(const R_Mesh *mesh, GFX_Device *device);

// Only binds index buffer as we use vertex pulling!!!
internal void R_MeshBind(const R_Mesh *mesh, const GFX_CmdBuffer *cmd);

internal void R_MeshDraw(const R_Mesh *mesh, const GFX_CmdBuffer *cmd);

internal inline u64
R_MeshVertexBufferSize(const R_Mesh *mesh)
{
	return mesh->vertex_count * mesh->vertex_stride;
}

internal inline u64
R_MeshIndexBufferSize(const R_Mesh *mesh)
{
	u64 index_stride = 0;

	switch (mesh->index_type)
	{
		case VK_INDEX_TYPE_UINT16:  index_stride = sizeof(u16); break;
		case VK_INDEX_TYPE_UINT32:  index_stride = sizeof(u32); break;
		default:                    AssertTrue(false);          break;
	}
	
	return mesh->index_count * index_stride;
}

#endif // RENDER_MESH_H
