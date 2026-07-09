
static void R_MeshAlloc(R_Mesh *mesh,
			u64 vertex_stride, VkIndexType index_type,
			u32 vertex_count, u32 index_count)
{
	mesh->vertex_stride = vertex_stride;
	mesh->index_type = index_type;
	
	mesh->vertex_count = vertex_count;
	mesh->index_count = index_count;

	G_BufferAllocInfo vertex_info = {0};
	vertex_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	vertex_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vertex_info.size = R_MeshVertexBufferSize(mesh);
	
	G_BufferAllocInfo index_info = {0};
	index_info.usage = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	index_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	index_info.size = R_MeshIndexBufferSize(mesh);

	mesh->vertex_buffer = G_DeviceBufferAlloc(&vertex_info);
	mesh->index_buffer  = G_DeviceBufferAlloc(&index_info);
}

static void R_MeshDestroy(const R_Mesh *mesh)
{
	G_DeviceBufferDestroy(mesh->vertex_buffer);
	G_DeviceBufferDestroy(mesh->index_buffer);
}

static void R_MeshWriteToStage(const R_Mesh *mesh,
				   G_BufferKey stage, u64 stage_base,
				   const void *vertices, const void *indices)
{
	u64 vb_size = R_MeshVertexBufferSize(mesh);
	u64 ib_size = R_MeshIndexBufferSize(mesh);

	G_DeviceBufferWrite(stage, vertices, vb_size, stage_base);
	G_DeviceBufferWrite(stage, indices,  ib_size, stage_base + vb_size);
}

static u64 R_MeshUpload(const R_Mesh *mesh, const G_CmdBuffer *cmd,
			 G_BufferKey stage, u64 stage_base)
{
	u64 vb_size = R_MeshVertexBufferSize(mesh);
	u64 ib_size = R_MeshIndexBufferSize(mesh);

	G_BufferCopy stage_to_vertex_copy = {0};
	stage_to_vertex_copy.src_offset = stage_base;
	stage_to_vertex_copy.dst_offset = 0;
	stage_to_vertex_copy.size = vb_size;

	G_BufferCopy stage_to_index_copy = {0};
	stage_to_index_copy.src_offset = stage_base + vb_size;
	stage_to_index_copy.dst_offset = 0;
	stage_to_index_copy.size = ib_size;

	G_CmdCopyBufferToBuffer(cmd, stage, mesh->vertex_buffer, 1, &stage_to_vertex_copy);
	G_CmdCopyBufferToBuffer(cmd, stage, mesh->index_buffer,  1, &stage_to_index_copy);

	return vb_size + ib_size;
}

static void R_MeshBindIndexBuffer(const R_Mesh *mesh, const G_CmdBuffer *cmd)
{
	G_CmdBindIndexBuffer(cmd, mesh->index_buffer, 0, VK_WHOLE_SIZE, mesh->index_type);
}

static void R_MeshDraw(const R_Mesh *mesh, const G_CmdBuffer *cmd)
{
	G_CmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

static void R_MeshDrawInstanced(const R_Mesh *mesh, const G_CmdBuffer *cmd, u32 first)
{
	G_CmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, first);
}
