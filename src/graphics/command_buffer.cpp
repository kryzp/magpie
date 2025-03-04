#include "command_buffer.h"
#include "backend.h"
#include "render_target.h"
#include "render_info.h"
#include "../common.h"

using namespace llt;

CommandBuffer CommandBuffer::fromGraphics()
{
	return CommandBuffer(g_vulkanBackend->m_graphicsQueue.getCurrentFrame().commandBuffer);
}

CommandBuffer CommandBuffer::fromCompute()
{
	return CommandBuffer(g_vulkanBackend->m_computeQueues[0].getCurrentFrame().commandBuffer);
}

CommandBuffer CommandBuffer::fromTransfer()
{
	//	if (m_transferQueues.size() > 0)
	//		return CommandBuffer(g_vulkanBackend->m_transferQueues[0].getCurrentFrame().commandBuffer);
	return fromGraphics();
}

CommandBuffer::CommandBuffer(VkCommandBuffer buffer)
	: m_buffer(buffer)
	, m_target(nullptr)
{
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::submit(VkSemaphore computeSemaphore)
{
	cauto &currentFrame = g_vulkanBackend->m_graphicsQueue.getCurrentFrame();

	LLT_VK_CHECK(
		vkEndCommandBuffer(m_buffer),
		"Failed to record command buffer"
	);

	int signalSemaphoreCount = 0;
	VkSemaphore queueFinishedSemaphores[] = { VK_NULL_HANDLE };

	int waitSemaphoreCount = 0;
	VkSemaphore blitSemaphore = VK_NULL_HANDLE;

	if (m_target)
	{
		if (m_target->getType() == RENDER_TARGET_TYPE_BACKBUFFER)
		{
			blitSemaphore = ((Backbuffer *)m_target)->getImageAvailableSemaphore();
			waitSemaphoreCount++;

			signalSemaphoreCount++;
			queueFinishedSemaphores[0] = ((Backbuffer *)m_target)->getRenderFinishedSemaphore();
		}
	}

	if (computeSemaphore)
		waitSemaphoreCount++;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };
	VkSemaphore waitForFinishSemaphores[] = { blitSemaphore, computeSemaphore };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_buffer;

	submitInfo.signalSemaphoreCount = signalSemaphoreCount;
	submitInfo.pSignalSemaphores = queueFinishedSemaphores;

	submitInfo.waitSemaphoreCount = waitSemaphoreCount;
	submitInfo.pWaitSemaphores = waitForFinishSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	vkResetFences(g_vulkanBackend->m_device, 1, &currentFrame.inFlightFence);

	LLT_VK_CHECK(
		vkQueueSubmit(g_vulkanBackend->m_graphicsQueue.getQueue(), 1, &submitInfo, currentFrame.inFlightFence),
		"Failed to submit draw command to buffer"
	);

	m_target = nullptr;
}

void CommandBuffer::beginRendering(GenericRenderTarget *target)
{
	m_target = target;

	beginRendering(m_target->getRenderInfo());
}

void CommandBuffer::beginRendering(const RenderInfo &info)
{
	_beginRecording();

	if (m_target)
		m_target->beginRendering(*this);
//	else
//		m_target = g_vulkanBackend->m_backbuffer;

	VkRenderingInfo renderInfo = info.getInfo();

	vkCmdBeginRendering(m_buffer, &renderInfo);
}

void CommandBuffer::endRendering()
{
	vkCmdEndRendering(m_buffer);

	if (m_target)
		m_target->endRendering(*this);
}

void CommandBuffer::_beginRecording()
{
	cauto &currentFrame = g_vulkanBackend->m_graphicsQueue.getCurrentFrame(); // current buffer comes from here! (note to myself tomorrow after i get sleep)

	vkWaitForFences(g_vulkanBackend->m_device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetCommandPool(g_vulkanBackend->m_device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	LLT_VK_CHECK(
		vkBeginCommandBuffer(m_buffer, &commandBufferBeginInfo),
		"Failed to begin recording command buffer"
	);
}

void CommandBuffer::beginCompute()
{
	cauto &currentFrame = g_vulkanBackend->m_computeQueues[0].getCurrentFrame(); // current buffer comes from here! (note to myself tomorrow after i get sleep)

	vkWaitForFences(g_vulkanBackend->m_device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetCommandPool(g_vulkanBackend->m_device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	LLT_VK_CHECK(
		vkBeginCommandBuffer(m_buffer, &commandBufferBeginInfo),
		"Failed to begin recording compute command buffer"
	);
}

void CommandBuffer::endCompute(VkSemaphore signalSemaphore)
{
	cauto &currentFrame = g_vulkanBackend->m_computeQueues[0].getCurrentFrame();

	LLT_VK_CHECK(
		vkEndCommandBuffer(m_buffer),
		"Failed to record compute command buffer"
	);

	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_buffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;

	vkResetFences(g_vulkanBackend->m_device, 1, &currentFrame.inFlightFence);

	LLT_VK_CHECK(
		vkQueueSubmit(g_vulkanBackend->m_computeQueues[0].getQueue(), 1, &submitInfo, currentFrame.inFlightFence),
		"Failed to submit compute command buffer"
	);
}

VkCommandBuffer CommandBuffer::getBuffer() const
{
	return m_buffer;
}
