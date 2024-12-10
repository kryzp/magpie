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
		QUEUE_FAMILY_GRAPHICS,
		QUEUE_FAMILY_COMPUTE,
		QUEUE_FAMILY_TRANSFER
	};

	class Queue
	{
	public:
		struct FrameData
		{
			VkFence inFlightFence;
			VkCommandPool commandPool;
			VkCommandBuffer commandBuffer;
		};

		Queue();
		~Queue();

		void init(VkQueue queue);
		void setData(QueueFamily family, uint32_t familyIdx);

		FrameData& getCurrentFrame();
		FrameData& getFrame(int idx);

		VkQueue getQueue() const;
		
		QueueFamily getFamily() const;
		Optional<uint32_t> getFamilyIdx() const;

	private:
		VkQueue m_queue;
		
		QueueFamily m_family;
		Optional<uint32_t> m_familyIdx;

		FrameData m_frames[mgc::FRAMES_IN_FLIGHT];
	};
}

#endif // VK_QUEUE_H
