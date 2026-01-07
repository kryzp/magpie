#include "queue.h"
#include "device.h"

using namespace gfx;

Queue::Queue()
	: device()
	, handle()
	, family_index()
	, frames{}
{
}

Queue::~Queue()
{
}

void Queue::destroy()
{
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		device->destroy_fence(frames[i].instant_submit_fence);
		device->destroy_command_pool(frames[i].command_pool);
	}
}

void Queue::wait_idle() const
{
	vkQueueWaitIdle(handle);
}

void Queue::next_frame()
{
	device->reset_command_pool(get_current_sync_data().command_pool);
}

void Queue::present(const Swapchain &swapchain, const VkSemaphore &wait)
{
	u32 image_index = swapchain.get_current_texture_index();
	VkSwapchainKHR swapchain_handle = swapchain.get_handle();
	VkSemaphore wait_semaphore = wait;

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = nullptr;
	present_info.pImageIndices = &image_index;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain_handle;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &wait_semaphore;

	VkResult result = vkQueuePresentKHR(handle, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		debug_log_crash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		debug_log_crash("Failed to present swapchain image.");
}

CommandBuffer Queue::begin_submit(VkFence fence)
{
	SyncData &current_sync = get_current_sync_data();

	fence = fence != VK_NULL_HANDLE ? fence : current_sync.instant_submit_fence;

	device->wait_for_fence(fence);
	device->reset_fence(fence);

	CommandBuffer cmd = current_sync.command_pool.fetch_free();
	
	cmd.begin();

	return cmd;
}

void Queue::end_submit(
	CommandBuffer &cmd,
	const VkSemaphoreSubmitInfo *signal,
	const VkSemaphoreSubmitInfo *wait,
	VkFence fence
)
{
	cmd.end();
	
	SyncData &current_sync = get_current_sync_data();

	VkCommandBufferSubmitInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = cmd.get_handle();

	VkSubmitInfo2 submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;

	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;

	if (signal) {
		submit_info.signalSemaphoreInfoCount = 1;
		submit_info.pSignalSemaphoreInfos = signal;
	} else {
		submit_info.signalSemaphoreInfoCount = 0;
		submit_info.pSignalSemaphoreInfos = nullptr;
	}

	if (wait) {
		submit_info.waitSemaphoreInfoCount = 1;
		submit_info.pWaitSemaphoreInfos = wait;
	} else {
		submit_info.waitSemaphoreInfoCount = 0;
		submit_info.pWaitSemaphoreInfos = nullptr;
	}

	fence = fence != VK_NULL_HANDLE ? fence : current_sync.instant_submit_fence;

	GFX_VK_CHECK(
		vkQueueSubmit2(handle, 1, &submit_info, fence),
		"Failed to submit command to queue."
	);
}

Queue::SyncData &Queue::get_current_sync_data()
{
	return frames[device->get_current_frame_index()];
}
