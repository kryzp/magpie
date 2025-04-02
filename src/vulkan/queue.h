#pragma once

#include <array>
#include <vector>

#include "third_party/volk.h"

namespace mgp
{
	class VulkanCore;

	class Queue
	{
	public:
		constexpr static int FRAMES_IN_FLIGHT = 3;

		class FrameData
		{
			constexpr static uint32_t INIT_COMMAND_BUFFER_COUNT = 4;

		public:
			FrameData();
			~FrameData() = default;

			void create(const VulkanCore *core, int queueFamilyIndex);
			void destroy() const;

			VkCommandBuffer getFreeBuffer();

			void resetCommandPool();

			const VkCommandPool &getCommandPool() const;

			const VkFence &getInFlightFence() const;
			const VkFence &getInstantSubmitFence() const;

		private:
			void expandBuffers(int n);

			VkCommandPool m_commandPool;

			unsigned m_freeIndex;
			std::vector<VkCommandBuffer> m_freeBuffers;

			VkFence m_inFlightFence;
			VkFence m_instantSubmitFence;

			const VulkanCore *m_core;
		};

		Queue();
		~Queue() = default;

		void create(const VulkanCore *core, unsigned index);
		void destroy();

		void setFamilyIndex(unsigned index);

		const VkQueue &getHandle() const;
		unsigned getFamilyIndex() const;

		FrameData &getFrame(int frame);
		const FrameData &getFrame(int frame) const;

	private:
		VkQueue m_queue;
		unsigned m_familyIndex;

		std::array<FrameData, FRAMES_IN_FLIGHT> m_frames;
	};
}
