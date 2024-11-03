#ifndef VK_QUEUE_H
#define VK_QUEUE_H

#include <vulkan/vulkan.h>

#include "../common.h"
#include "../container/optional.h"

namespace llt
{
	enum QueueFamily
	{
		QUEUE_FAMILY_NONE,
		QUEUE_FAMILY_PRESENT_GRAPHICS,
		QUEUE_FAMILY_COMPUTE,
		QUEUE_FAMILY_TRANSFER
	};

	class Queue
	{
		struct FrameData
		{
			VkFence inFlightFence;
			VkCommandPool commandPool;
			VkCommandBuffer commandBuffer;
		};

	public:
		Queue();
		~Queue();

		void init(VkQueue queue);
		void setData(QueueFamily family, uint32_t familyIdx);

		void nextFrame();

		FrameData& getCurrentFrame();
		FrameData& getFrame(int idx);

		int getCurrentFrameIdx() const;

		VkQueue getQueue() const;
		
		QueueFamily getFamily() const;
		Optional<uint32_t> getFamilyIdx() const;

	private:
		VkQueue m_queue;
		
		QueueFamily m_family;
		Optional<uint32_t> m_familyIdx;

		FrameData m_frames[mgc::FRAMES_IN_FLIGHT];
		uint64_t m_currentFrameIdx;
	};
}

#endif // VK_QUEUE_H
