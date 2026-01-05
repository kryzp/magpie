#pragma once

#include <volk/volk.h>

#include "core/types.h"

namespace gfx
{

class Queue {
	friend class Device;

public:
	Queue();
	~Queue();

	void wait_idle();

	void submit(const VkSubmitInfo2 &submit_info, VkFence fence);
	VkResult present(const VkPresentInfoKHR &present_info);

	const VkQueue &get_handle() const
	{
		return handle;
	}

	u32 get_family_index() const
	{
		return family_index;
	}

private:
	VkQueue handle;
	u32 family_index;
};

}
