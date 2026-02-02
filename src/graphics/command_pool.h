#pragma once

#include <volk/volk.h>

#include "core/types.h"
#include "container/vector.h"

namespace gfx
{
	class CommandBuffer;

	class CommandPool {
		friend class Device;

	public:
		static constexpr u32 MAX_BUFFERS = 64;

		CommandPool();
		~CommandPool();

		const VkCommandPool &get_handle() const
		{
			return handle;
		}

	private:
		VkCommandPool handle;
		u32 used_count;
		Vector<VkCommandBuffer> buffers;
	};
}
