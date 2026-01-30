#include "queue.h"
#include "device.h"

using namespace gfx;

Queue::Queue()
	: device()
	, handle()
	, family_index()
	, timeline_semaphore()
	, timeline_value()
	, frames{}
{
}

Queue::~Queue()
{
}

void Queue::destroy()
{
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
		device->destroy_command_pool(frames[i].command_pool);

	device->destroy_semaphore(timeline_semaphore);
}

void Queue::wait_idle() const
{
	vkQueueWaitIdle(handle);
}

void Queue::wait_until(u64 value) const
{
	VkSemaphoreWaitInfo wait_info = {};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores = &timeline_semaphore;
	wait_info.pValues = &value;

	GFX_VK_CHECK(
		vkWaitSemaphores(
			device->get_handle(),
			&wait_info, UINT64_MAX
		),
		"Failed to wait on timeline semaphore"
	);
}

void Queue::reset_pool()
{
	device->reset_command_pool(get_current_sync_data().command_pool);
}

CommandBuffer Queue::get_command_buffer()
{
	CommandBuffer cmd = get_current_sync_data().command_pool.fetch_free();
	cmd.begin();
	return cmd;
}

u64 Queue::submit(
	CommandBuffer &cmd,
	const Vector<VkSemaphoreSubmitInfo> &waits,
	const Vector<VkSemaphoreSubmitInfo> &signals,
	VkFence fence
)
{
	cmd.end();

	timeline_value++;

	VkSemaphoreSubmitInfo timeline_signal_info = {};
	timeline_signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	timeline_signal_info.semaphore = timeline_semaphore;
	timeline_signal_info.value = timeline_value; // Signal the N+1 value!!
	timeline_signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	// TODO: THIS FUCKING SUCKS!!!
	Vector<VkSemaphoreSubmitInfo> all_signals = signals;
	all_signals.push_back(timeline_signal_info);
	
	VkCommandBufferSubmitInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = cmd.get_handle();

	VkSubmitInfo2 submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;

	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;

	submit_info.waitSemaphoreInfoCount = waits.size();
	submit_info.pWaitSemaphoreInfos = waits.data();

	submit_info.signalSemaphoreInfoCount = all_signals.size();
	submit_info.pSignalSemaphoreInfos = all_signals.data();

	GFX_VK_CHECK(
		vkQueueSubmit2(handle, 1, &submit_info, fence),
		"Failed to submit command to queue."
	);

	return timeline_value;
}

void Queue::submit_immediate(const std::function<void(CommandBuffer &cmd)> &record)
{
	wait_idle();
	CommandBuffer cmd = get_command_buffer();
	record(cmd);
	wait_until(submit(cmd, {}, {}, VK_NULL_HANDLE));
}

void Queue::present(
	const Swapchain &swapchain,
	const Vector<VkSemaphore> &waits
)
{
	u32 image_index = swapchain.get_current_texture_index();
	VkSwapchainKHR swapchain_handle = swapchain.get_handle();

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = nullptr;
	present_info.pImageIndices = &image_index;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain_handle;
	present_info.waitSemaphoreCount = waits.size();
	present_info.pWaitSemaphores = waits.data();

	VkResult result = vkQueuePresentKHR(handle, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		debug_log_crash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		debug_log_crash("Failed to present swapchain image.");
}

u64 Queue::get_timeline_value() const
{
	return timeline_value;
}

u64 Queue::get_completed_timeline_value() const
{
	u64 result = 0;

	vkGetSemaphoreCounterValue(
		device->get_handle(),
		timeline_semaphore,
		&result
	);

	return result;
}

Queue::SyncData &Queue::get_current_sync_data()
{
	return frames[device->get_current_frame_index()];
}
