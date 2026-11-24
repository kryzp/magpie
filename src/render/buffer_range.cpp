#include "render/buffer_range.h"
#include "graphics/device.h"

void *rnd::BufferRange::Map() const
{
	return (void *)((u8 *)gfx::Device::Get()->BufferMap(buffer) + offset);
}

u64 rnd::BufferRange::Address() const
{
	return gfx::Device::Get()->BufferAddress(buffer) + offset;
}
