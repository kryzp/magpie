
internal Mesh MeshInit(VertexFormat *format, u32 vertex_count, void *vertices,
		       u32 index_count, u16 *indices)
{
	Mesh mesh = {0};
	mesh.vertex_format = format;
	mesh.vertex_count = vertex_count;
	mesh.index_count = index_count;

	u64 vertex_buffer_size = vertex_count * format->vertex_size;
	u64 index_buffer_size = index_count * sizeof(u16);

	mesh.vertex_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
					    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
					    vertex_buffer_size);

	mesh.index_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
					   index_buffer_size);

	GPUBuffer staging_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						  vertex_buffer_size + index_buffer_size);
	{
		GPUBufferWrite(&staging_buffer, vertices, vertex_buffer_size, 0);
		GPUBufferWrite(&staging_buffer, indices, index_buffer_size, vertex_buffer_size);

		CommandBuffer cmd = BeginGraphicsInstantSubmit();
		{
			VkBufferCopy stage_to_vertex_copy = {0};
			stage_to_vertex_copy.srcOffset = 0;
			stage_to_vertex_copy.dstOffset = 0;
			stage_to_vertex_copy.size = vertex_buffer_size;

			CmdCopyBufferToBuffer(&cmd, &staging_buffer,
					      &mesh.vertex_buffer, 1,
					      &stage_to_vertex_copy);

			VkBufferCopy stage_to_index_copy = {0};
			stage_to_index_copy.srcOffset = vertex_buffer_size;
			stage_to_index_copy.dstOffset = 0;
			stage_to_index_copy.size = index_buffer_size;

			CmdCopyBufferToBuffer(&cmd, &staging_buffer,
					      &mesh.index_buffer, 1,
					      &stage_to_index_copy);
		}
		EndGraphicsInstantSubmit(&cmd);
	}
	GraphicsWaitIdle();
	GPUBufferDestroy(&staging_buffer);

	return mesh;
}

internal void MeshDestroy(Mesh *mesh)
{
	GPUBufferDestroy(&mesh->vertex_buffer);
	GPUBufferDestroy(&mesh->index_buffer);
}

internal void MeshBindCmd(Mesh *mesh, CommandBuffer *cmd)
{
	CmdBindVertexBuffer(cmd, 0, &mesh->vertex_buffer, 0);
	CmdBindIndexBuffer(cmd, &mesh->index_buffer, 0);
}

internal void MeshDrawCmd(Mesh *mesh, CommandBuffer *cmd)
{
	CmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

internal void MeshDrawCmdID(Mesh *mesh, CommandBuffer *cmd, u32 instance_id)
{
	CmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, instance_id);
}

// ---

internal SubModel *ModelCreateSubModel(Model *model)
{
	SubModel *sub_model = MemoryArenaPush(model->arena, sizeof(SubModel));
	sub_model->next = model->sub_models;
	sub_model->parent = model;

	model->sub_models = sub_model;
	model->sub_model_count++;

	return sub_model;
}

internal void ModelDestroy(Model *model)
{
	for (i32 i = 0; i < model->sub_model_count; i++)
		MeshDestroy(&model->sub_models[i].mesh);
}
