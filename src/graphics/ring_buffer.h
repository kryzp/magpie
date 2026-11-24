#ifndef GRAPHICS_RING_BUFFER_H
#define GRAPHICS_RING_BUFFER_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "graphics/buffer.h"

namespace gfx {

struct Alloc {
	u64 offset = 0;
	u64 size = 0;
	void *cpu = nullptr;
	u64 gpu = 0;
};

// basically a GPU arena over a persistently mapped buffer. the slices share
// the parent's buffer, so the buffer is only freed by an explicit destroy()
// on the ring that allocated it.
class RingBuffer {
public:
	RingBuffer();
	explicit RingBuffer(const BufferAllocInfo &alloc_info);

	void Destroy();

	RingBuffer SliceView(u64 offset, u64 size) const;

	void Reset();

	Alloc Push(u64 bytes, u64 alignment);
	Alloc PushCopy(const void *src, u64 bytes, u64 alignment);

	template <typename T>
	Alloc PushArray(u64 count)
	{
		return Push(sizeof(T) * count, alignof(T));
	}

	template <typename T>
	Alloc PushCopyArray(const T *src, u64 count)
	{
		return PushCopy(src, sizeof(T) * count, alignof(T));
	}

	void *AddrCPU(u64 offset) const;
	u64 AddrGPU(u64 offset) const;

	ResourceKey buffer;
	u64 base_offset;
	u64 used;
	u64 capacity;
	u64 high_water; // largest 'used' seen so far
	void *base_cpu;
	u64 base_gpu;
};

}

#endif // GRAPHICS_RING_BUFFER_H
