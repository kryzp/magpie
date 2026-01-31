#include "model.h"

using namespace gfx;

void Mesh::init(
	Device *device, u64 vertex_size,
	u32 vertex_count, void *vertices,
	u32 index_count, u32 *indices
)
{
	this->device = device;
	this->vertex_size = vertex_size;
	this->vertex_count = vertex_count;
	this->index_count = index_count;

	u64 vertex_buffer_size = vertex_count * vertex_size;
	u64 index_buffer_size = index_count * sizeof(u32);

	GpuBuffer *staging_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		vertex_buffer_size + index_buffer_size
	);

	staging_buffer->write(vertices, vertex_buffer_size, 0);
	staging_buffer->write(indices, index_buffer_size, vertex_buffer_size);

	this->vertex_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		vertex_buffer_size
	);

	this->index_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		index_buffer_size
	);

	device->graphics().submit_immediate([&](CommandBuffer &cmd) -> void {
		VkBufferCopy stage_to_vertex_copy = {};
		stage_to_vertex_copy.srcOffset = 0;
		stage_to_vertex_copy.dstOffset = 0;
		stage_to_vertex_copy.size = vertex_buffer_size;

		VkBufferCopy stage_to_index_copy = {};
		stage_to_index_copy.srcOffset = vertex_buffer_size;
		stage_to_index_copy.dstOffset = 0;
		stage_to_index_copy.size = index_buffer_size;

		cmd.copy_buffer_to_buffer(staging_buffer, vertex_buffer, { stage_to_vertex_copy });
		cmd.copy_buffer_to_buffer(staging_buffer, index_buffer, { stage_to_index_copy });
	});

	device->destroy_buffer(staging_buffer);
}

void Mesh::destroy() const
{
	device->destroy_buffer(vertex_buffer);
	device->destroy_buffer(index_buffer);
}
