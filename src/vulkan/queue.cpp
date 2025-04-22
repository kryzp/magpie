#include "queue.h"

#include "core/common.h"

#include "core.h"

using namespace mgp;

void Queue::FrameData::create(const VulkanCore *core, int queueFamilyIndex)
{
	m_core = core;

	// create dynamic pool
	pool.create(m_core, queueFamilyIndex);

	// create in flight fence
	VkFenceCreateInfo inFlightFenceCreateInfo = {};
	inFlightFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	inFlightFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	MGP_VK_CHECK(
		vkCreateFence(
			m_core->getLogicalDevice(),
			&inFlightFenceCreateInfo,
			nullptr,
			&this->inFlightFence
		),
		"Failed to create queue frame in flight fence"
	);

	// create in instant submit fence
	VkFenceCreateInfo instantSubmitFenceCreateInfo = {};
	instantSubmitFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	instantSubmitFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	MGP_VK_CHECK(
		vkCreateFence(
			m_core->getLogicalDevice(),
			&instantSubmitFenceCreateInfo,
			nullptr,
			&this->instantSubmitFence
		),
		"Failed to create queue frame instant submit fence"
	);
}

void Queue::FrameData::destroy() const
{
	pool.destroy();

	vkDestroyFence(m_core->getLogicalDevice(), inFlightFence, nullptr);
	vkDestroyFence(m_core->getLogicalDevice(), instantSubmitFence, nullptr);
}

Queue::Queue()
	: m_queue(VK_NULL_HANDLE)
	, m_familyIndex(0)
	, m_frames()
{
}

void Queue::create(const VulkanCore *core, unsigned index)
{
	vkGetDeviceQueue(
		core->getLogicalDevice(),
		m_familyIndex,
		index,
		&m_queue
	);

	for (auto &f : m_frames)
		f.create(core, m_familyIndex);
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
	createInfo.queueCount = (unsigned)priorities.size();
	createInfo.pQueuePriorities = priorities.data();

	return createInfo;
}

void Queue::setFamilyIndex(unsigned index)
{
	m_familyIndex = index;
}

const VkQueue &Queue::getHandle() const
{
	return m_queue;
}

unsigned Queue::getFamilyIndex() const
{
	return m_familyIndex;
}

Queue::FrameData &Queue::getFrame(int frame)
{
	return m_frames[frame];
}

const Queue::FrameData &Queue::getFrame(int frame) const
{
	return m_frames[frame];
}
