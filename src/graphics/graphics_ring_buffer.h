#ifndef GRAPHICS_RING_BUFFER_H
#define GRAPHICS_RING_BUFFER_H

typedef struct GFX_Alloc GFX_Alloc;
struct GFX_Alloc
{
	u64 offset;
	u64 size;
	void *cpu;
	u64   gpu;
};

typedef struct GFX_RingBuffer GFX_RingBuffer;
struct GFX_RingBuffer
{
	GFX_BufferKey buffer;
	u64 used;
	u64 capacity;
	void *base_cpu;
	u64   base_gpu;
};

internal GFX_RingBuffer GFX_RingBufferAlloc(GFX_Device *device, const GFX_BufferAllocInfo *alloc_info);

internal void GFX_RingBufferDestroy(const GFX_RingBuffer *ring, GFX_Device *device);

internal void GFX_RingBufferReset(GFX_RingBuffer *ring);

internal GFX_Alloc GFX_RingBufferPush(GFX_RingBuffer *ring, u64 bytes, u64 alignment);

#define GFX_RingBufferPushArray(ring, type, count) GFX_RingBufferPush((ring), sizeof(type) * (count), alignof(type))

internal void *GFX_RingBufferAddrCPU(const GFX_RingBuffer *ring, u64 offset);
internal u64   GFX_RingBufferAddrGPU(const GFX_RingBuffer *ring, u64 offset);

#endif // GRAPHICS_RING_BUFFER_H
