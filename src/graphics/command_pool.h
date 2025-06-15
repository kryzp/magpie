#pragma once

#include <vector>

#include <Volk/volk.h>

namespace mgp
{
	class GraphicsCore;

	class CommandPoolDynamic
	{
		constexpr static uint32_t INIT_COMMAND_BUFFER_COUNT = 4;

	public:
		CommandPoolDynamic();
		~CommandPoolDynamic();

		void create(GraphicsCore *gfx, int queueFamilyIndex);
		void destroy() const;

		void reset();

		VkCommandBuffer getFreeBuffer();

		const VkCommandPool &getCommandPool() const;

	private:
		GraphicsCore *m_gfx;

		void expandBuffers(int n);

		VkCommandPool m_commandPool;

		unsigned m_freeIndex;
		std::vector<VkCommandBuffer> m_freeBuffers;
	};
}
