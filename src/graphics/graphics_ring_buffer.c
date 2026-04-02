
internal GFX_RingBuffer
GFX_RingBufferAlloc(GFX_Device *device, const GFX_BufferAllocInfo *alloc_info)
{
	GFX_RingBuffer ring = {0};
	ring.buffer = GFX_DeviceBufferAlloc(device, alloc_info);
	ring.capacity = alloc_info->size;
	ring.used = 0;

	GFX_Buffer *gfx_buffer = GFX_DeviceBufferFromKey(device, ring.buffer);
	
	ring.base_cpu = GFX_BufferMap(gfx_buffer);
	ring.base_gpu = GFX_BufferAddress(gfx_buffer);

	return ring;
}

internal void
GFX_RingBufferDestroy(const GFX_RingBuffer *ring, GFX_Device *device)
{
	GFX_DeviceBufferDestroy(device, ring->buffer);
}

internal void
GFX_RingBufferReset(GFX_RingBuffer *ring)
{
	ring->used = 0;
}

internal GFX_Alloc
GFX_RingBufferPush(GFX_RingBuffer *ring, u64 bytes, u64 alignment)
{
	ring->used = MemAlignUp(ring->used, alignment);

	if (ring->used + bytes > ring->capacity)
		AssertTrue(false);

	GFX_Alloc alloc = {0};
	alloc.offset = ring->used;
	alloc.size = bytes;
	alloc.cpu = GFX_RingBufferAddrCPU(ring, ring->used);
	alloc.gpu = GFX_RingBufferAddrGPU(ring, ring->used);

	ring->used += bytes;

	return alloc;
}

internal void *
GFX_RingBufferAddrCPU(const GFX_RingBuffer *ring, u64 offset)
{
	return (void *)((u8 *)ring->base_cpu + offset);
}

internal u64
GFX_RingBufferAddrGPU(const GFX_RingBuffer *ring, u64 offset)
{
	return ring->base_gpu + offset;
}
