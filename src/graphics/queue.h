#pragma once

#include <volk/volk.h>

#include "core/types.h"

#include "command_pool.h"

namespace gfx
{

class CommandBuffer;
class Swapchain;
class Device;

constexpr u32 FRAMES_IN_FLIGHT = 3;

class Queue {
	friend class Device;

	struct SyncData {
		VkFence instant_submit_fence;
		CommandPool command_pool;
	};

public:
	Queue();
	~Queue();

	void destroy();

	void wait_idle() const;

	void next_frame();

	void present(const Swapchain &swapchain, const VkSemaphore &wait);

	CommandBuffer begin_submit(VkFence fence);

	void end_submit(
		CommandBuffer &cmd,
		const VkSemaphoreSubmitInfo *signal,
		const VkSemaphoreSubmitInfo *wait,
		VkFence fence
	);

	const VkQueue &get_handle() const
	{
		return handle;
	}

	u32 get_family_index() const
	{
		return family_index;
	}

private:
	SyncData &get_current_sync_data();

	Device *device;

	VkQueue handle;
	u32 family_index;

	SyncData frames[FRAMES_IN_FLIGHT];
};

}
