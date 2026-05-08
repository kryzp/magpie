#ifndef GRAPHICS_BUFFER_H
#define GRAPHICS_BUFFER_H

typedef struct GFX_Buffer GFX_Buffer;
struct GFX_Buffer
{
	VkBuffer vk_handle;
	VkBufferUsageFlags2 usage;

	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
	VmaAllocationCreateFlags allocation_flags;

	u64 size;

	u64 device_address;
};

internal b32 GFX_BufferIsStorage (const GFX_Buffer *buffer);
internal b32 GFX_BufferIsUniform (const GFX_Buffer *buffer);

#endif // GRAPHICS_BUFFER_H
