#include "queue.h"

#include "core/common.h"

#include "core.h"

using namespace mgp;

Queue::FrameData::FrameData()
	: m_commandPool(VK_NULL_HANDLE)
	, m_freeIndex(0)
	, m_freeBuffers()
	, m_inFlightFence(VK_NULL_HANDLE)
	, m_instantSubmitFence(VK_NULL_HANDLE)
	, m_core(nullptr)
{
}

void Queue::FrameData::create(const VulkanCore *core, int queueFamilyIndex)
{
	m_core = core;

	// create command pools
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndex;

	MGP_VK_CHECK(
		vkCreateCommandPool(
			m_core->getLogicalDevice(),
			&createInfo,
			nullptr,
			&m_commandPool
		),
		"Failed to create queue frame command pool"
	);

	// allocate the initial amount of command buffers
	expandBuffers(INIT_COMMAND_BUFFER_COUNT);

	// create in flight fence
	VkFenceCreateInfo inFlightFenceCreateInfo = {};
	inFlightFenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	inFlightFenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	MGP_VK_CHECK(
		vkCreateFence(
			m_core->getLogicalDevice(),
			&inFlightFenceCreateInfo,
			nullptr,
			&m_inFlightFence
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
			&m_instantSubmitFence
		),
		"Failed to create queue frame instant submit fence"
	);
}

void Queue::FrameData::destroy() const
{
	vkDestroyCommandPool(m_core->getLogicalDevice(), m_commandPool, nullptr);

	vkDestroyFence(m_core->getLogicalDevice(), m_inFlightFence, nullptr);
	vkDestroyFence(m_core->getLogicalDevice(), m_instantSubmitFence, nullptr);
}

VkCommandBuffer Queue::FrameData::getFreeBuffer()
{
	if (m_freeIndex >= m_freeBuffers.size())
	{
		// initially allocate INIT_COMMAND_BUFFER_COUNT command buffers
		// as they get used up (submitted) throughout the frame we go through them all
		// each frame we reset them all
		// if during the frame we use them all up and need more still, we
		// generate double our current amount

		expandBuffers(m_freeBuffers.size() * 2);
	}

	return m_freeBuffers[m_freeIndex++];
}

void Queue::FrameData::resetCommandPool()
{
	vkResetCommandPool(m_core->getLogicalDevice(), m_commandPool, 0);
	m_freeIndex = 0;
}

const VkCommandPool &Queue::FrameData::getCommandPool() const
{
	return m_commandPool;
}

const VkFence &Queue::FrameData::getInFlightFence() const
{
	return m_inFlightFence;
}

const VkFence &Queue::FrameData::getInstantSubmitFence() const
{
	return m_instantSubmitFence;
}

void Queue::FrameData::expandBuffers(int n)
{
	uint64_t oldSize = m_freeBuffers.size();
	uint64_t count = n - m_freeBuffers.size();

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = count;
	commandBufferAllocateInfo.commandPool = m_commandPool;

	m_freeBuffers.resize(n);

	MGP_VK_CHECK(
		vkAllocateCommandBuffers(
			m_core->getLogicalDevice(),
			&commandBufferAllocateInfo,
			m_freeBuffers.data() + oldSize
		),
		"Failed to create queue frame command buffer"
	);
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
