#ifndef GFX_BUFFER_H
#define GFX_BUFFER_H

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/core_types.h"

enum gfx_buffer_access_type {
	GFX_BUFFER_ACCESS_TYPE_undefined,
	GFX_BUFFER_ACCESS_TYPE_graphics_rw,
	GFX_BUFFER_ACCESS_TYPE_compute_rw,
	GFX_BUFFER_ACCESS_TYPE_copy_src,
	GFX_BUFFER_ACCESS_TYPE_copy_dst,
	GFX_BUFFER_ACCESS_TYPE_indirect,
	GFX_BUFFER_ACCESS_TYPE_max_enum
};

struct gfx_buffer_access {
	VkImageLayout layout;
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
};

struct gfx_buffer {
	VkBuffer handle;
	VkBufferUsageFlags2 usage;

	enum gfx_buffer_access_type access_type;

	u64 size;

	VkDeviceAddress device_address;

	VmaAllocator *allocator;
	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
	VmaAllocationCreateFlagBits allocation_flags;
};

void gfx_buffer_read(struct gfx_buffer *buffer, void *dst, u64 length, u64 offset);
void gfx_buffer_write(struct gfx_buffer *buffer, void *src, u64 length, u64 offset);

//void gfx_buffer_write_aligned(struct gfx_buffer *buffer, void *src, u64 stride, u64 index);

void *gfx_buffer_map_data(struct gfx_buffer *buffer);

bool gfx_buffer_is_storage(struct gfx_buffer *buffer);
bool gfx_buffer_is_uniform(struct gfx_buffer *buffer);

#endif // GFX_BUFFER_H
