
internal void
R_MeshAlloc(R_Mesh *mesh, GFX_Device *device,
			u64 vertex_stride, VkIndexType index_type,
			u32 vertex_count, u32 index_count)
{
	mesh->vertex_stride = vertex_stride;
	mesh->index_type = index_type;
	
	mesh->vertex_count = vertex_count;
	mesh->index_count = index_count;

	GFX_BufferAllocInfo vertex_info = {0};
	vertex_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	vertex_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vertex_info.size = R_MeshVertexBufferSize(mesh);
	
	GFX_BufferAllocInfo index_info = {0};
	index_info.usage = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	index_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	index_info.size = R_MeshIndexBufferSize(mesh);

	mesh->vertex_buffer = GFX_DeviceBufferAlloc(device, &vertex_info);
	mesh->index_buffer  = GFX_DeviceBufferAlloc(device, &index_info);
}

internal void
R_MeshDestroy(const R_Mesh *mesh, GFX_Device *device)
{
	GFX_DeviceBufferDestroy(device, mesh->vertex_buffer);
	GFX_DeviceBufferDestroy(device, mesh->index_buffer);
}

internal void
R_MeshWriteToStage(const R_Mesh *mesh, GFX_Device *device,
				   GFX_BufferKey stage, u64 stage_base,
				   void *vertices, void *indices)
{
	u64 vb_size = R_MeshVertexBufferSize(mesh);
	u64 ib_size = R_MeshIndexBufferSize(mesh);

	GFX_DeviceBufferWrite(device, stage, vertices, vb_size, stage_base);
	GFX_DeviceBufferWrite(device, stage, indices,  ib_size, stage_base + vb_size);
}

internal u64
R_MeshUpload(const R_Mesh *mesh, const GFX_CmdBuffer *cmd,
			 GFX_BufferKey stage, u64 stage_base)
{
	u64 vb_size = R_MeshVertexBufferSize(mesh);
	u64 ib_size = R_MeshIndexBufferSize(mesh);

	VkBufferCopy2 stage_to_vertex_copy = {0};
	stage_to_vertex_copy.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
	stage_to_vertex_copy.srcOffset = stage_base;
	stage_to_vertex_copy.dstOffset = 0;
	stage_to_vertex_copy.size = vb_size;

	VkBufferCopy2 stage_to_index_copy = {0};
	stage_to_index_copy.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
	stage_to_index_copy.srcOffset = stage_base + vb_size;
	stage_to_index_copy.dstOffset = 0;
	stage_to_index_copy.size = ib_size;

	GFX_CmdCopyBufferToBuffer(cmd, stage, mesh->vertex_buffer, 1, &stage_to_vertex_copy);
	GFX_CmdCopyBufferToBuffer(cmd, stage, mesh->index_buffer,  1, &stage_to_index_copy);

	return vb_size + ib_size;
}

internal void
R_MeshBind(const R_Mesh *mesh, const GFX_CmdBuffer *cmd)
{
	GFX_CmdBindIndexBuffer(cmd, mesh->index_buffer, 0, VK_WHOLE_SIZE, mesh->index_type);
}

internal void
R_MeshDraw(const R_Mesh *mesh, const GFX_CmdBuffer *cmd)
{
	GFX_CmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}
