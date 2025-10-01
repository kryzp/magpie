#include "buffer.h"

void gfx_buffer_read(struct gfx_buffer *buffer, void *dst, u64 length, u64 offset)
{
	vmaCopyAllocationToMemory(*buffer->allocator, buffer->allocation, offset, dst, length);
}

void gfx_buffer_write(struct gfx_buffer *buffer, void *src, u64 length, u64 offset)
{
	vmaCopyMemoryToAllocation(*buffer->allocator, src, buffer->allocation, offset, length);
}

void *gfx_buffer_map_data(struct gfx_buffer *buffer)
{
	return buffer->allocation_info.pMappedData;
}

bool gfx_buffer_is_storage(struct gfx_buffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0;
}

bool gfx_buffer_is_uniform(struct gfx_buffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT) != 0;
}
