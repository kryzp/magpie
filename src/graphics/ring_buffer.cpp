#include "graphics/ring_buffer.h"
#include "graphics/device.h"
#include "core/maths.h"

gfx::RingBuffer::RingBuffer()
	: base_offset(0)
	, used(0)
	, capacity(0)
	, high_water(0)
	, base_cpu(nullptr)
	, base_gpu(0)
{
}

gfx::RingBuffer::RingBuffer(const BufferAllocInfo &alloc_info)
	: RingBuffer()
{
	Device *device = Device::Get();

	buffer = device->BufferAlloc(alloc_info);
	base_offset = 0;
	capacity = alloc_info.size;
	used = 0;

	base_cpu = device->BufferMap(buffer);
	base_gpu = device->BufferAddress(buffer);
}

void gfx::RingBuffer::Destroy()
{
	if (!buffer.IsNull())
		Device::Get()->BufferDestroy(buffer);

	buffer = ResourceKey::Null();
}

gfx::RingBuffer gfx::RingBuffer::SliceView(u64 offset, u64 size) const
{
	AssertTrue(offset + size <= capacity);

	RingBuffer slice;
	slice.buffer = buffer;
	slice.base_offset = base_offset + offset;
	slice.capacity = size;
	slice.base_cpu = (u8 *)base_cpu + offset;
	slice.base_gpu = base_gpu + offset;

	return slice;
}

void gfx::RingBuffer::Reset()
{
	high_water = maths::Max(high_water, used);
	used = 0;
}

gfx::Alloc gfx::RingBuffer::Push(u64 bytes, u64 alignment)
{
	Alloc alloc;

	alignment = maths::Max(alignment, (u64)16);

	u64 offset = MemAlignUp(used, alignment);

	if (offset + bytes > capacity) {
		DebugLogE(
			Device::Get()->log_channel,
			"Upload ring out of space (offset + bytes > capacity) :: (%llu + %llu > %llu). Raise the frame upload budget.",
			offset, bytes, capacity
		);

		return alloc;
	}

	alloc.offset = base_offset + offset;
	alloc.size = bytes;
	alloc.cpu = AddrCPU(offset);
	alloc.gpu = AddrGPU(offset);

	used = offset + bytes;

	return alloc;
}

gfx::Alloc gfx::RingBuffer::PushCopy(const void *src, u64 bytes, u64 alignment)
{
	Alloc alloc = Push(bytes, alignment);

	if (alloc.cpu && bytes > 0)
		memcpy(alloc.cpu, src, bytes);

	return alloc;
}

void *gfx::RingBuffer::AddrCPU(u64 offset) const
{
	return (void *)((u8 *)base_cpu + offset);
}

u64 gfx::RingBuffer::AddrGPU(u64 offset) const
{
	return base_gpu + offset;
}
