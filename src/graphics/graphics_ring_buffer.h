#ifndef GRAPHICS_RING_BUFFER_H
#define GRAPHICS_RING_BUFFER_H

typedef struct G_Alloc G_Alloc;
struct G_Alloc
{
	u64 offset;
	u64 size;
	void *cpu;
	u64   gpu;
};

typedef struct G_RingBuffer G_RingBuffer;
struct G_RingBuffer
{
	G_BufferKey buffer;
	u64 used;
	u64 capacity;
	void *base_cpu;
	u64   base_gpu;
};

static G_RingBuffer G_RingBufferAlloc(G_Device *device, const G_BufferAllocInfo *alloc_info);

static void G_RingBufferDestroy(const G_RingBuffer *ring, G_Device *device);

static void G_RingBufferReset(G_RingBuffer *ring);

static G_Alloc G_RingBufferPush(G_RingBuffer *ring, u64 bytes, u64 alignment);

#define G_RingBufferPushArray(ring, type, count) G_RingBufferPush((ring), sizeof(type) * (count), _Alignof(type))

static void *G_RingBufferAddrCPU(const G_RingBuffer *ring, u64 offset);
static u64   G_RingBufferAddrGPU(const G_RingBuffer *ring, u64 offset);

#endif // GRAPHICS_RING_BUFFER_H
