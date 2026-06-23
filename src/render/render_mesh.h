#ifndef RENDER_MESH_H
#define RENDER_MESH_H

typedef struct R_Mesh R_Mesh;
struct R_Mesh
{
	u64 vertex_stride;
	VkIndexType index_type;

	u32 vertex_count;
	u32 index_count;

	G_BufferKey vertex_buffer;
	G_BufferKey index_buffer;
};

static void R_MeshAlloc(R_Mesh *mesh, G_Device *device,
						  u64 vertex_stride, VkIndexType index_type,
						  u32 vertex_count, u32 index_count);

static void R_MeshDestroy(const R_Mesh *mesh, G_Device *device);

static void R_MeshWriteToStage(const R_Mesh *mesh, G_Device *device,
								 G_BufferKey stage, u64 stage_base,
								 const void *vertices, const void *indices);

static u64 R_MeshUpload(const R_Mesh *mesh, const G_CmdBuffer *cmd,
						  G_BufferKey stage, u64 stage_base);

//static u64 R_MeshVertexAddress(const R_Mesh *mesh, G_Device *device);

// Only binds index buffer as we use vertex pulling!!!
static void R_MeshBind(const R_Mesh *mesh, const G_CmdBuffer *cmd);

static void R_MeshDraw          (const R_Mesh *mesh, const G_CmdBuffer *cmd);
static void R_MeshDrawInstanced (const R_Mesh *mesh, const G_CmdBuffer *cmd, u32 first);

static inline u64 R_MeshVertexBufferSize(const R_Mesh *mesh)
{
	return mesh->vertex_count * mesh->vertex_stride;
}

static inline u64 R_MeshIndexBufferSize(const R_Mesh *mesh)
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
