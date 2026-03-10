#include "model.h"

using namespace gfx;

Mesh::Mesh()
	: device(nullptr)
	, vertex_size(0)
	, vertex_count(0)
	, index_count(0)
	, vertex_buffer(nullptr)
	, index_buffer(nullptr)
{
}

Mesh::~Mesh()
{
}

void Mesh::create_buffers(
	Device *device, u64 vertex_size,
	u32 vertex_count, u32 index_count
)
{
	this->device = device;
	this->vertex_size = vertex_size;
	this->vertex_count = vertex_count;
	this->index_count = index_count;

	this->vertex_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		get_vertex_buffer_size()
	);

	this->index_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		get_index_buffer_size()
	);
}

void Mesh::destroy_buffers() const
{
	device->destroy_buffer(vertex_buffer);
	device->destroy_buffer(index_buffer);
}

void Mesh::write_to_staging_buffer(
	GpuBuffer *staging_buffer, u64 offset,
	void *vertices, IndexType *indices
)
{
	u64 vertex_buffer_size = get_vertex_buffer_size();
	u64 index_buffer_size = get_index_buffer_size();

	staging_buffer->write(vertices, vertex_buffer_size, offset);
	staging_buffer->write(indices, index_buffer_size, offset + vertex_buffer_size);
}

u64 Mesh::batch_upload(
	CommandBuffer &cmd,
	GpuBuffer *staging_buffer, u64 offset
)
{
	u64 vertex_buffer_size = get_vertex_buffer_size();
	u64 index_buffer_size = get_index_buffer_size();

	VkBufferCopy stage_to_vertex_copy = {};
	stage_to_vertex_copy.srcOffset = offset;
	stage_to_vertex_copy.dstOffset = 0;
	stage_to_vertex_copy.size = vertex_buffer_size;

	VkBufferCopy stage_to_index_copy = {};
	stage_to_index_copy.srcOffset = offset + vertex_buffer_size;
	stage_to_index_copy.dstOffset = 0;
	stage_to_index_copy.size = index_buffer_size;

	cmd.copy_buffer_to_buffer(staging_buffer, vertex_buffer, { stage_to_vertex_copy });
	cmd.copy_buffer_to_buffer(staging_buffer, index_buffer, { stage_to_index_copy });

	return vertex_buffer_size + index_buffer_size;
}
