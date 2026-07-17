
static G_RingBuffer G_RingBufferAlloc(const G_BufferAllocInfo *alloc_info)
{
	G_RingBuffer ring = {0};
	ring.buffer = G_DeviceBufferAlloc(alloc_info);
	ring.capacity = alloc_info->size;
	ring.used = 0;

	ring.base_cpu = G_DeviceBufferMap(ring.buffer);
	ring.base_gpu = G_DeviceBufferAddress(ring.buffer);

	return ring;
}

static void G_RingBufferDestroy(const G_RingBuffer *ring)
{
	G_DeviceBufferDestroy(ring->buffer);
}

static void G_RingBufferReset(G_RingBuffer *ring)
{
	ring->used = 0;
}

static G_Alloc G_RingBufferPush(G_RingBuffer *ring, u64 bytes, u64 alignment)
{
	ring->used = MemAlignUp(ring->used, alignment);

	if (ring->used + bytes > ring->capacity)
		ring->used = 0;
	
	G_Alloc alloc = {0};
	alloc.offset = ring->used;
	alloc.size = bytes;
	alloc.cpu = G_RingBufferAddrCPU(ring, ring->used);
	alloc.gpu = G_RingBufferAddrGPU(ring, ring->used);

	ring->used += bytes;

	return alloc;
}

static void *G_RingBufferAddrCPU(const G_RingBuffer *ring, u64 offset)
{
	return (void *)((u8 *)ring->base_cpu + offset);
}

static u64 G_RingBufferAddrGPU(const G_RingBuffer *ring, u64 offset)
{
	return ring->base_gpu + offset;
}
