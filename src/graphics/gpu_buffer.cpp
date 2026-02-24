#include "gpu_buffer.h"

#include "device.h"

using namespace gfx;

void GpuBuffer::read(void *dst, u64 length, u64 offset)
{
	vmaCopyAllocationToMemory(allocator, allocation, offset, dst, length);
}

void GpuBuffer::write(const void *src, u64 length, u64 offset)
{
	vmaCopyMemoryToAllocation(allocator, src, allocation, offset, length);
}

uptr GpuBuffer::map() const
{
	return (uptr)allocation_info.pMappedData;
}

bool GpuBuffer::is_storage() const
{
	return (usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0;
}

bool GpuBuffer::is_uniform() const
{
	return (usage & VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT) != 0;
}
