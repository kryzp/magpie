#include "queue.h"
#include "device.h"

void gfx_queue_wait_idle(struct gfx_queue *queue)
{
	vkQueueWaitIdle(queue->handle);
}

void gfx_queue_submit(struct gfx_queue *queue, const VkSubmitInfo2 *submit_info, VkFence fence)
{
	GFX_VK_CHECK(vkQueueSubmit2(queue->handle, 1, submit_info, fence),
		     "Failed to submit command to queue.");
}

VkResult gfx_queue_present(struct gfx_queue *queue, const VkPresentInfoKHR *present_info)
{
	return vkQueuePresentKHR(queue->handle, present_info);
}
