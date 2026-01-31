#include "command_pool.h"
#include "device.h"

using namespace gfx;

CommandPool::CommandPool()
	: handle(VK_NULL_HANDLE)
	, free_index(0)
	, family_index(0)
	, free_buffers{}
{
}

CommandPool::~CommandPool()
{
}

CommandBuffer CommandPool::fetch_free()
{
	assert(free_index < MAX_BUFFERS);
	return CommandBuffer(free_buffers[free_index++]);
}
