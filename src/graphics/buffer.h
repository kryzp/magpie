#ifndef GRAPHICS_BUFFER_H
#define GRAPHICS_BUFFER_H

#include "core/types.h"

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace gfx {
struct BufferAllocInfo {
	VkBufferUsageFlags2 usage = 0;
	VmaAllocationCreateFlags flags = 0;
	u64 size = 0;
};

struct Buffer {
	VkBuffer vk_handle = VK_NULL_HANDLE;
	VkBufferUsageFlags2 usage = 0;

	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info = {};
	VmaAllocationCreateFlags allocation_flags = 0;

	u64 size = 0;
	u64 device_address = 0;

	bool IsStorage() const
	{
		return (usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0;
	}

	bool IsUniform() const
	{
		return (usage & VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT) != 0;
	}
};
}

#endif // GRAPHICS_BUFFER_H
