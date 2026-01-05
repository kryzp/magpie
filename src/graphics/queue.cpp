#include "queue.h"
#include "device.h"

using namespace gfx;

Queue::Queue()
{
}

Queue::~Queue()
{
}

void Queue::wait_idle()
{
	vkQueueWaitIdle(handle);
}

void Queue::submit(const VkSubmitInfo2 &submit_info, VkFence fence)
{
	GFX_VK_CHECK(
		vkQueueSubmit2(handle, 1, &submit_info, fence),
		"Failed to submit command to queue."
	);
}

VkResult Queue::present(const VkPresentInfoKHR &present_info)
{
	return vkQueuePresentKHR(handle, &present_info);
}
