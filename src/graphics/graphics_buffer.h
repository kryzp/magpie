#ifndef GRAPHICS_BUFFER_H
#define GRAPHICS_BUFFER_H

typedef struct GFX_Buffer GFX_Buffer;
struct GFX_Buffer
{
	VkBuffer handle;
	VkBufferUsageFlags2 usage;

	VmaAllocator allocator;
	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
	VmaAllocationCreateFlags allocation_flags;

	u64 size;

	u64 device_address;
};

internal void GFX_BufferRead(const GFX_Buffer *buffer, void *dst, u64 length, u64 offset);
internal void GFX_BufferWrite(const GFX_Buffer *buffer, const void *src, u64 length, u64 offset);

internal void *GFX_BufferMap(const GFX_Buffer *buffer);
internal u64   GFX_BufferAddress(const GFX_Buffer *buffer);

internal b32 GFX_BufferIsStorage(const GFX_Buffer *buffer);
internal b32 GFX_BufferIsUniform(const GFX_Buffer *buffer);

typedef struct GFX_BufferRange GFX_BufferRange;
struct GFX_BufferRange
{
	const GFX_Buffer *buffer;
	u64 size;
	u64 offset;
};

internal inline
void *GFX_BufferRangeMap(const GFX_BufferRange *range)
{
	return (void *)((u8 *)GFX_BufferMap(range->buffer) + range->offset);
}

internal inline
u64 GFX_BufferRangeAddress(const GFX_BufferRange *range)
{
	return GFX_BufferAddress(range->buffer)  + range->offset;
}

#endif // GRAPHICS_BUFFER_H
