#pragma once

#include <volk/volk.h>

#include "core/types.h"

namespace gfx
{

class CommandBuffer;

class CommandPool {
	friend class Device;

public:
	static constexpr u32 MAX_BUFFERS = 64;

	CommandPool();
	~CommandPool();

	CommandBuffer fetch_free();

	const VkCommandPool &get_handle() const
	{
		return handle;
	}

	u32 get_family_index() const
	{
		return family_index;
	}

private:
	VkCommandPool handle;
	u32 family_index;
	u32 free_index;
	VkCommandBuffer free_buffers[MAX_BUFFERS];
};

}
