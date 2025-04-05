#include "command_pool.h"

#include "core/common.h"

#include "vulkan/core.h"

using namespace mgp;

CommandPoolDynamic::CommandPoolDynamic()
	: m_commandPool(VK_NULL_HANDLE)
	, m_freeIndex(0)
	, m_freeBuffers()
	, m_core(nullptr)
{
}

CommandPoolDynamic::~CommandPoolDynamic()
{
}

void CommandPoolDynamic::create(const VulkanCore *core, int queueFamilyIndex)
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
}

void CommandPoolDynamic::destroy() const
{
	vkDestroyCommandPool(m_core->getLogicalDevice(), m_commandPool, nullptr);
}

VkCommandBuffer CommandPoolDynamic::getFreeBuffer()
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

void CommandPoolDynamic::reset()
{
	vkResetCommandPool(m_core->getLogicalDevice(), m_commandPool, 0);
	m_freeIndex = 0;
}

const VkCommandPool &CommandPoolDynamic::getCommandPool() const
{
	return m_commandPool;
}

void CommandPoolDynamic::expandBuffers(int n)
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
