#pragma once

#include <array>

#include <Volk/volk.h>

#include "command_pool.h"

namespace mgp
{
	class VulkanCore;

	class Queue
	{
	public:
		constexpr static int FRAMES_IN_FLIGHT = 3;

		struct FrameData
		{
			void create(const VulkanCore *core, int queueFamilyIndex);
			void destroy() const;

			CommandPoolDynamic pool;

			VkFence inFlightFence;
			VkFence instantSubmitFence;

		private:
			const VulkanCore *m_core;
		};

		Queue();
		~Queue() = default;

		void create(const VulkanCore *core, unsigned index);
		void destroy();

		VkDeviceQueueCreateInfo getCreateInfo(const std::vector<float> &priorities);

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
