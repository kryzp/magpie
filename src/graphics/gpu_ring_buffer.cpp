#include "gpu_ring_buffer.h"

#include "device.h"

using namespace gfx;

GpuRingBuffer::GpuRingBuffer()
	: device(nullptr)
	, buffer(nullptr)
	, used(0)
{
}

GpuRingBuffer::~GpuRingBuffer()
{
}

void GpuRingBuffer::allocate(Device *device, VkBufferUsageFlags2 usage, VmaAllocationCreateFlags flags, u64 size)
{
	this->device = device;
	this->buffer = device->alloc_buffer(usage, flags, size);
	this->used = 0;
}

void GpuRingBuffer::destroy() const
{
	device->destroy_buffer(buffer);
}

void GpuRingBuffer::reset()
{
	used = 0;
}

GpuAlloc<u8> GpuRingBuffer::push(u64 bytes, u64 alignment)
{
	used = memory_align_up(used, alignment);

	if (used + bytes > buffer->capacity())
		assert(0);
//		used = 0;

	GpuAlloc<u8> alloc = {};
	alloc.offset = used;
	alloc.size = bytes;
	alloc.cpu = (u8 *)cpu_addr(used);
	alloc.gpu = gpu_addr(used);

	used += bytes;

	return alloc;
}

GpuBuffer *GpuRingBuffer::get_buffer()
{
	return buffer;
}

const GpuBuffer *GpuRingBuffer::get_buffer() const
{
	return buffer;
}

uptr GpuRingBuffer::cpu_addr(uptr offset) const
{
	return buffer->map() + offset;
}

u64 GpuRingBuffer::gpu_addr(u64 offset) const
{
	return buffer->get_device_address() + offset;
}
