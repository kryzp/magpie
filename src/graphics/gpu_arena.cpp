#include "gpu_arena.h"

#include "device.h"

using namespace gfx;

GpuArena::GpuArena()
	: device(nullptr)
	, buffer(nullptr)
	, used(0)
{
}

GpuArena::~GpuArena()
{
}

void GpuArena::allocate(Device *device, VkBufferUsageFlags2 usage, VmaAllocationCreateFlags flags, u64 size)
{
	this->device = device;

	buffer = device->alloc_buffer(usage, flags, size);
	used = 0;
}

void GpuArena::destroy() const
{
	device->destroy_buffer(buffer);
}

void GpuArena::reset()
{
	used = 0;
}

GpuArenaAlloc GpuArena::push(u64 size, u64 alignment)
{
	u64 aligned = memory_align_up(used, alignment);
	
	assert(aligned + size <= buffer->get_size());

	used = aligned + size;

	GpuArenaAlloc alloc = {};
	alloc.offset = aligned;
	alloc.cpu = base_cpu(aligned);
	alloc.gpu = base_gpu(aligned);

	return alloc;
}

GpuBuffer *GpuArena::get_buffer()
{
	return buffer;
}

const GpuBuffer *GpuArena::get_buffer() const
{
	return buffer;
}

uptr GpuArena::base_cpu(uptr offset) const
{
	return buffer->map() + offset;
}

u64 GpuArena::base_gpu(u64 offset) const
{
	return buffer->get_device_address() + offset;
}
