#include "command_pool.h"
#include "device.h"

using namespace gfx;

CommandPool::CommandPool()
	: handle(VK_NULL_HANDLE)
	, used_count(0)
	, buffers()
{
}

CommandPool::~CommandPool()
{
}
