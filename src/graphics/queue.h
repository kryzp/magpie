#pragma once

#include <volk/volk.h>

#include <functional>

#include "core/types.h"
#include "container/vector.h"

#include "command_pool.h"

namespace gfx
{
	class CommandBuffer;
	class Swapchain;
	class Device;

	constexpr u32 FRAMES_IN_FLIGHT = 3;

	class Queue {
		friend class Device;

	public:
		Queue();
		~Queue();

		void destroy();

		void reset_pool();

		void wait_idle() const;
		void wait_until(u64 value) const;

		CommandBuffer get_command_buffer();

		// Returns the new timeline value that marks this frames' completion.
		u64 submit(
			CommandBuffer &cmd,
			const Vector<VkSemaphoreSubmitInfo> &waits,
			const Vector<VkSemaphoreSubmitInfo> &signals,
			VkFence fence
		);

		void submit_immediate(const std::function<void(CommandBuffer &cmd)> &record);

		void present(
			const Swapchain &swapchain,
			const Vector<VkSemaphore> &waits
		);

		u64 get_timeline_value() const;
		u64 get_completed_timeline_value() const;

		const VkQueue &get_handle() const
		{
			return handle;
		}

		u32 get_family_index() const
		{
			return family_index;
		}

	private:
		Device *device;

		VkQueue handle;
		u32 family_index;

		VkSemaphore timeline_semaphore;
		u64 timeline_value;

		struct SyncData {
			CommandPool command_pool;
		};

		SyncData &get_current_sync_data();

		SyncData frames[FRAMES_IN_FLIGHT];
	};
}
