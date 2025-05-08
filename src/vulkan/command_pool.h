#pragma once

#include <vector>

#include <Volk/volk.h>

namespace mgp
{
	class VulkanCore;

	class CommandPoolDynamic
	{
		constexpr static uint32_t INIT_COMMAND_BUFFER_COUNT = 4;

	public:
		CommandPoolDynamic();
		~CommandPoolDynamic();

		void create(const VulkanCore *core, int queueFamilyIndex);
		void destroy() const;

		void reset();

		VkCommandBuffer getFreeBuffer();

		const VkCommandPool &getCommandPool() const;

	private:
		void expandBuffers(int n);

		VkCommandPool m_commandPool;

		unsigned m_freeIndex;
		std::vector<VkCommandBuffer> m_freeBuffers;

		const VulkanCore *m_core;
	};
}
