#pragma once

#include <inttypes.h>

#include <vector>
#include <array>

#include <Volk/volk.h>

#include "command_pool.h"
#include "constants.h"

namespace mgp
{
	class GraphicsCore;

	class Queue
	{
	public:
		struct FrameData
		{
			void create(GraphicsCore *gfx, int queueFamilyIndex);
			void destroy() const;

			CommandPoolDynamic pool;

			VkFence inFlightFence;
			VkFence instantSubmitFence;

		private:
			GraphicsCore *m_gfx;
		};

		Queue();
		~Queue() = default;

		void create(GraphicsCore *gfx, uint32_t index);
		void destroy();

		VkDeviceQueueCreateInfo getCreateInfo(const std::vector<float> &priorities);

		void setFamilyIndex(uint32_t index);
		uint32_t getFamilyIndex() const;

		const VkQueue &getHandle() const;

		FrameData &getFrame(int frame);
		const FrameData &getFrame(int frame) const;

	private:
		VkQueue m_queue;
		uint32_t m_familyIndex;

		std::array<FrameData, gfx_constants::FRAMES_IN_FLIGHT> m_frames;
	};
}
