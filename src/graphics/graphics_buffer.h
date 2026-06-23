#ifndef GRAPHICS_BUFFER_H
#define GRAPHICS_BUFFER_H

typedef struct G_Buffer G_Buffer;
struct G_Buffer
{
	VkBuffer vk_handle;
	VkBufferUsageFlags2 usage;

	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
	VmaAllocationCreateFlags allocation_flags;

	u64 size;

	u64 device_address;
};

static b32 G_BufferIsStorage (const G_Buffer *buffer);
static b32 G_BufferIsUniform (const G_Buffer *buffer);

#endif // GRAPHICS_BUFFER_H
