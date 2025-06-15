#include "queue.h"

#include "core/common.h"

#include "graphics_core.h"

using namespace mgp;

void Queue::FrameData::create(GraphicsCore *gfx, int queueFamilyIndex)
{
	m_gfx = gfx;

	// create dynamic pool
	pool.create(m_gfx, queueFamilyIndex);

	// create in flight fence
	{
		VkFenceCreateInfo inFlightFenceCreateInfo = {};
		inFlightFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		inFlightFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		MGP_VK_CHECK(
			vkCreateFence(
				m_gfx->getLogicalDevice(),
				&inFlightFenceCreateInfo,
				nullptr,
				&this->inFlightFence
			),
			"Failed to create queue frame in flight fence"
		);
	}

	// create in instant submit fence
	{
		VkFenceCreateInfo instantSubmitFenceCreateInfo = {};
		instantSubmitFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		instantSubmitFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		MGP_VK_CHECK(
			vkCreateFence(
				m_gfx->getLogicalDevice(),
				&instantSubmitFenceCreateInfo,
				nullptr,
				&this->instantSubmitFence
			),
			"Failed to create queue frame instant submit fence"
		);
	}
}

void Queue::FrameData::destroy() const
{
	pool.destroy();

	vkDestroyFence(m_gfx->getLogicalDevice(), inFlightFence, nullptr);
	vkDestroyFence(m_gfx->getLogicalDevice(), instantSubmitFence, nullptr);
}

Queue::Queue()
	: m_queue(VK_NULL_HANDLE)
	, m_familyIndex(0)
	, m_frames()
{
}

void Queue::create(GraphicsCore *gfx, uint32_t index)
{
	vkGetDeviceQueue(
		gfx->getLogicalDevice(),
		m_familyIndex,
		index,
		&m_queue
	);

	for (auto &f : m_frames)
		f.create(gfx, m_familyIndex);
}

void Queue::destroy()
{
	for (cauto &f : m_frames)
		f.destroy();
}

VkDeviceQueueCreateInfo Queue::getCreateInfo(const std::vector<float> &priorities)
{
	VkDeviceQueueCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	createInfo.queueFamilyIndex = m_familyIndex;
	createInfo.queueCount = (uint32_t)priorities.size();
	createInfo.pQueuePriorities = priorities.data();

	return createInfo;
}

void Queue::setFamilyIndex(uint32_t index)
{
	m_familyIndex = index;
}

uint32_t Queue::getFamilyIndex() const
{
	return m_familyIndex;
}

const VkQueue &Queue::getHandle() const
{
	return m_queue;
}

Queue::FrameData &Queue::getFrame(int frame)
{
	return m_frames[frame];
}

const Queue::FrameData &Queue::getFrame(int frame) const
{
	return m_frames[frame];
}
