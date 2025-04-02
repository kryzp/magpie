#include "sync.h"

#include "core/common.h"

#include "core.h"
#include "swapchain.h"

using namespace mgp;

InFlightSync::InFlightSync(VulkanCore *core)
	: m_core(core)
	, m_cmd(VK_NULL_HANDLE)
{
	m_swapchain = core->createSwapchain();
	m_swapchain->setClearColour(Colour::black());
}

void InFlightSync::destroy()
{
	delete m_swapchain;
}

CommandBuffer &InFlightSync::begin()
{
	auto &currentFrame = m_core->getGraphicsQueue().getFrame(m_core->getCurrentFrameIndex());

	vkQueueWaitIdle(m_core->getGraphicsQueue().getHandle());
	currentFrame.resetCommandPool();

	m_core->waitForFence(currentFrame.getInFlightFence());
	m_core->resetFence(currentFrame.getInFlightFence());

	m_swapchain->acquireNextImage();

	m_cmd = CommandBuffer(currentFrame.getFreeBuffer());

	m_cmd.begin();

	m_swapchain->beginRendering(m_cmd);

	return m_cmd;
}

void InFlightSync::present()
{
	m_swapchain->endRendering(m_cmd);

	m_cmd.end();

	VkFence fence = m_core->getGraphicsQueue().getFrame(m_core->getCurrentFrameIndex()).getInFlightFence();

	VkSemaphoreSubmitInfo imageAvailableSemaphore = m_swapchain->getImageAvailableSemaphoreSubmitInfo();
	VkSemaphoreSubmitInfo renderFinishedSemaphore = m_swapchain->getRenderFinishedSemaphoreSubmitInfo();

	VkCommandBufferSubmitInfo bufferInfo = m_cmd.getSubmitInfo();

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.flags = 0;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &bufferInfo;

	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &renderFinishedSemaphore;

	submitInfo.waitSemaphoreInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = &imageAvailableSemaphore;
	
	MGP_VK_CHECK(
		vkQueueSubmit2(m_core->getGraphicsQueue().getHandle(), 1, &submitInfo, fence),
		"Failed to submit in-flight draw command to buffer"
	);

	VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo = m_swapchain->getRenderFinishedSemaphoreSubmitInfo();
	unsigned imageIndex = m_swapchain->getCurrentSwapchainImageIndex();

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphoreSubmitInfo.semaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain->getHandle();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(m_core->getGraphicsQueue().getHandle(), &presentInfo);

	// rebuild swapchain if failure
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		m_swapchain->rebuildSwapchain();
	}

	// just crash if the error still not a success but not known
	else if (result != VK_SUCCESS)
	{
		MGP_ERROR("Failed to present swap chain image: %d", result);
	}
}

Swapchain *InFlightSync::getSwapchain()
{
	return m_swapchain;
}

const Swapchain *InFlightSync::getSwapchain() const
{
	return m_swapchain;
}

void InFlightSync::onWindowResize(int w, int h)
{
	m_swapchain->onWindowResize(w, h);
}

InstantSubmitSync::InstantSubmitSync(VulkanCore *core)
	: m_core(core)
	, m_cmd(core->getGraphicsQueue().getFrame(core->getCurrentFrameIndex()).getFreeBuffer())
{
}

CommandBuffer &InstantSubmitSync::begin()
{
	cauto &currentFrame = m_core->getGraphicsQueue().getFrame(m_core->getCurrentFrameIndex());

	m_core->waitForFence(currentFrame.getInstantSubmitFence());
	m_core->resetFence(currentFrame.getInstantSubmitFence());

	m_cmd.begin();
	return m_cmd;
}

void InstantSubmitSync::submit()
{
	m_cmd.end();

	VkFence fence = m_core->getGraphicsQueue().getFrame(m_core->getCurrentFrameIndex()).getInstantSubmitFence();

	VkCommandBufferSubmitInfo bufferInfo = m_cmd.getSubmitInfo();

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.flags = 0;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &bufferInfo;

	submitInfo.signalSemaphoreInfoCount = 0;
	submitInfo.waitSemaphoreInfoCount = 0;

	MGP_VK_CHECK(
		vkQueueSubmit2(m_core->getGraphicsQueue().getHandle(), 1, &submitInfo, fence),
		"Failed to submit instant draw command to buffer"
	);
}
