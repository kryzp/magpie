
internal G_RingBuffer G_RingBufferAlloc(const G_BufferAllocInfo *alloc_info)
{
	G_RingBuffer ring = {0};
	ring.buffer = G_BufferAlloc(alloc_info);
	ring.base_offset = 0;
	ring.capacity = alloc_info->size;
	ring.used = 0;

	ring.base_cpu = G_BufferMap(ring.buffer);
	ring.base_gpu = G_BufferAddress(ring.buffer);

	return ring;
}

internal void G_RingBufferDestroy(const G_RingBuffer *ring)
{
	G_BufferDestroy(ring->buffer);
}

internal G_RingBuffer G_RingBufferSliceView(const G_RingBuffer *parent, u64 offset, u64 size)
{
	AssertTrue(offset + size <= parent->capacity);

	G_RingBuffer slice = {0};
	slice.buffer = parent->buffer;
	slice.base_offset = parent->base_offset + offset;
	slice.capacity = size;
	slice.base_cpu = (u8 *)parent->base_cpu + offset;
	slice.base_gpu = parent->base_gpu + offset;

	return slice;
}

internal void G_RingBufferReset(G_RingBuffer *ring)
{
	ring->high_water = MaxValue(ring->high_water, ring->used);
	ring->used = 0;
}

internal G_Alloc G_RingBufferPush(G_RingBuffer *ring, u64 bytes, u64 alignment)
{
	G_Alloc alloc = {0};

	alignment = MaxValue(alignment, 16);

	u64 offset = MemAlignUp(ring->used, alignment);

	if (offset + bytes > ring->capacity)
	{
		DebugLogE(g_device->log_channel,
				  "Upload ring out of space (offset + bytes > capacity) :: (%llu + %llu > %llu). Raise the frame upload budget.",
				  offset, bytes, ring->capacity);
		
		return alloc;
	}

	alloc.offset = ring->base_offset + offset;
	alloc.size = bytes;
	alloc.cpu = G_RingBufferAddrCPU(ring, offset);
	alloc.gpu = G_RingBufferAddrGPU(ring, offset);

	ring->used = offset + bytes;

	return alloc;
}

internal G_Alloc G_RingBufferPushCopy(G_RingBuffer *ring, const void *src, u64 bytes, u64 alignment)
{
	G_Alloc alloc = G_RingBufferPush(ring, bytes, alignment);

	if (alloc.cpu && bytes > 0)
		MemCopy(alloc.cpu, src, bytes);

	return alloc;
}

internal void *G_RingBufferAddrCPU(const G_RingBuffer *ring, u64 offset)
{
	return (void *)((u8 *)ring->base_cpu + offset);
}

internal u64 G_RingBufferAddrGPU(const G_RingBuffer *ring, u64 offset)
{
	return ring->base_gpu + offset;
}
