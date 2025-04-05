#pragma once

#include "command_buffer.h"

namespace mgp
{
	class VulkanCore;
	class Swapchain;

	class InFlightSync
	{
	public:
		InFlightSync(VulkanCore *core);
		~InFlightSync() = default;

		void destroy();

		CommandBuffer &begin();
		void present(const VkSemaphore *waitOn = nullptr); // should be a list

		/*
		CommandBuffer &beginCompute();
		void dispatch(const VkSemaphore *signal = nullptr);
		*/

		Swapchain *getSwapchain();
		const Swapchain *getSwapchain() const;

		void onWindowResize(int w, int h);

	private:
		VulkanCore *m_core;
		Swapchain *m_swapchain;

		CommandBuffer m_cmd;
	};

	class InstantSubmitSync
	{
	public:
		InstantSubmitSync(VulkanCore *core);
		~InstantSubmitSync() = default;

		CommandBuffer &begin();
		void submit();

	private:
		const VulkanCore *m_core;
		
		CommandBuffer m_cmd;
	};
}
