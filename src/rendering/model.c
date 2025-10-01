#include "model.h"

void gfx_mesh_init(struct gfx_mesh *mesh, struct gfx_device *device,
		   u64 vertex_size,
		   u32 vertex_count, void *vertices,
		   u32 index_count, u16 *indices)
{
	mesh->vertex_size = vertex_size;
	mesh->vertex_count = vertex_count;
	mesh->index_count = index_count;

	u64 vertex_buffer_size = vertex_count * vertex_size;
	u64 index_buffer_size = index_count * sizeof(u16);

	mesh->vertex_buffer = gfx_device_buffer_alloc(device,
						     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
						     VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
						     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						     vertex_buffer_size);

	mesh->index_buffer = gfx_device_buffer_alloc(device,
						    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
						    VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
						    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						    index_buffer_size);

	struct gfx_buffer staging_buffer = gfx_device_buffer_alloc(device,
								   VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
								   vertex_buffer_size + index_buffer_size);
	
	gfx_buffer_write(&staging_buffer, vertices, vertex_buffer_size, 0);
	gfx_buffer_write(&staging_buffer, indices, index_buffer_size, vertex_buffer_size);

	struct gfx_command_buffer cmd = gfx_device_begin_instant_submit(device);

	VkBufferCopy stage_to_vertex_copy = {0};
	stage_to_vertex_copy.srcOffset = 0;
	stage_to_vertex_copy.dstOffset = 0;
	stage_to_vertex_copy.size = vertex_buffer_size;

	gfx_cmd_copy_buffer_to_buffer(&cmd,
				      &staging_buffer,
				      &mesh->vertex_buffer,
				      1, &stage_to_vertex_copy);

	VkBufferCopy stage_to_index_copy = {0};
	stage_to_index_copy.srcOffset = vertex_buffer_size;
	stage_to_index_copy.dstOffset = 0;
	stage_to_index_copy.size = index_buffer_size;

	gfx_cmd_copy_buffer_to_buffer(&cmd,
				      &staging_buffer,
				      &mesh->index_buffer,
				      1, &stage_to_index_copy);
		
	gfx_device_end_instant_submit(device, &cmd);

	gfx_device_wait_idle(device);
	gfx_device_buffer_destroy(device, &staging_buffer);
}

void gfx_mesh_destroy(struct gfx_mesh *mesh, struct gfx_device *device)
{
	gfx_device_buffer_destroy(device, &mesh->vertex_buffer);
	gfx_device_buffer_destroy(device, &mesh->index_buffer);
}

void gfx_mesh_bind_indices(struct gfx_mesh *mesh, struct gfx_command_buffer *cmd)
{
	gfx_cmd_bind_index_buffer(cmd, &mesh->index_buffer, 0);
}

void gfx_mesh_draw_indexed(struct gfx_mesh *mesh, struct gfx_command_buffer *cmd)
{
	gfx_cmd_draw_indexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

void gfx_mesh_draw_indexed_id(struct gfx_mesh *mesh, struct gfx_command_buffer *cmd, u32 instance_id)
{
	gfx_cmd_draw_indexed(cmd, mesh->index_count, 1, 0, 0, instance_id);
}

void gfx_model_init(struct gfx_model *model)
{
}

void gfx_model_destroy(struct gfx_model *model, struct gfx_device *device)
{
	for (struct gfx_sub_model *sub = model->sub_models; sub; sub = sub->next)
		gfx_mesh_destroy(&sub->mesh, device);
}

struct gfx_sub_model *gfx_model_add_sub_model(struct gfx_model *model)
{
	struct gfx_sub_model *sub_model = memory_arena_push(model->arena, sizeof(struct gfx_sub_model));
	sub_model->next = model->sub_models;
	sub_model->parent = model;
	model->sub_models = sub_model;

	return sub_model;
}
