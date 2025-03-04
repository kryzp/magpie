#include "command_buffer.h"

#include "core/common.h"

#include "backend.h"
#include "render_target.h"
#include "render_info.h"
#include "shader.h"
#include "pipeline.h"

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

void CommandBuffer::_beginRecording()
{
	cauto &currentFrame = g_vulkanBackend->m_graphicsQueue.getCurrentFrame();

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

void CommandBuffer::endRendering()
{
	vkCmdEndRendering(m_buffer);

	if (m_target)
		m_target->endRendering(*this);
}

void CommandBuffer::drawIndexed(
	uint32_t indexCount,
	uint32_t instanceCount,
	uint32_t firstIndex,
	int32_t vertexOffset,
	uint32_t firstInstance
)
{
	vkCmdDrawIndexed(
		m_buffer,
		indexCount,
		instanceCount,
		firstIndex,
		vertexOffset,
		firstInstance
	);
}

void CommandBuffer::drawIndexedIndirect(
	VkBuffer buffer,
	VkDeviceSize offset,
	uint32_t drawCount,
	uint32_t stride
)
{
	vkCmdDrawIndexedIndirect(
		m_buffer,
		buffer,
		offset,
		drawCount,
		stride
	);
}

void CommandBuffer::drawIndexedIndirectCount(
	VkBuffer buffer,
	VkDeviceSize offset,
	VkBuffer countBuffer,
	VkDeviceSize countBufferOffset,
	uint32_t maxDrawCount,
	uint32_t stride
)
{
	vkCmdDrawIndexedIndirectCount(
		m_buffer,
		buffer,
		offset,
		countBuffer,
		countBufferOffset,
		maxDrawCount,
		stride
	);
}

void CommandBuffer::bindDescriptorSets(
	VkPipelineBindPoint bindPoint,
	VkPipelineLayout pipelineLayout,
	uint32_t firstSet,
	uint32_t setCount,
	const VkDescriptorSet *descriptorSets,
	uint32_t dynamicOffsetCount,
	const uint32_t *dynamicOffsets
)
{
	vkCmdBindDescriptorSets(
		m_buffer,
		bindPoint, pipelineLayout,
		firstSet,
		setCount, descriptorSets,
		dynamicOffsetCount,
		dynamicOffsets
	);
}

void CommandBuffer::bindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline)
{
	vkCmdBindPipeline(
		m_buffer,
		bindPoint,
		pipeline
	);
}

void CommandBuffer::bindPipeline(Pipeline &pipeline)
{
	vkCmdBindPipeline(
		m_buffer,
		pipeline.getBindPoint(),
		pipeline.getPipeline()
	);
}

void CommandBuffer::setViewports(
	uint32_t viewportCount,
	VkViewport *viewports,
	uint32_t firstViewport
)
{
	vkCmdSetViewport(m_buffer, firstViewport, viewportCount, viewports);
}

void CommandBuffer::setViewport(const VkViewport &viewport)
{
	vkCmdSetViewport(m_buffer, 0, 1, &viewport);
}

void CommandBuffer::setViewport(const RenderInfo &info)
{
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = (float)info.getHeight();
	viewport.width = (float)info.getWidth();
	viewport.height = -(float)info.getHeight();
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(m_buffer, 0, 1, &viewport);
}

void CommandBuffer::setScissors(
	uint32_t scissorCount,
	VkRect2D *scissors,
	uint32_t firstScissor
)
{
	vkCmdSetScissor(m_buffer, firstScissor, scissorCount, scissors);
}

void CommandBuffer::setScissor(const VkRect2D &scissor)
{
	vkCmdSetScissor(m_buffer, 0, 1, &scissor);
}

void CommandBuffer::setScissor(const RenderInfo &info)
{
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { info.getWidth(), info.getHeight() };

	vkCmdSetScissor(m_buffer, 0, 1, &scissor);
}

void CommandBuffer::pushConstants(
	VkPipelineLayout pipelineLayout,
	VkShaderStageFlagBits stageFlags,
	uint32_t offset,
	uint32_t size,
	void *data
)
{
	vkCmdPushConstants(
		m_buffer,
		pipelineLayout,
		stageFlags,
		offset,
		size,
		data
	);
}

void CommandBuffer::pushConstants(
	VkPipelineLayout pipelineLayout,
	VkShaderStageFlagBits stageFlags,
	ShaderParameters &params
)
{
	cauto &data = params.getPackedConstants();

	vkCmdPushConstants(
		m_buffer,
		pipelineLayout,
		stageFlags,
		0,
		data.size(),
		data.data()
	);
}

void CommandBuffer::bindVertexBuffers(
	uint32_t firstBinding,
	uint32_t count,
	VkBuffer *buffers,
	VkDeviceSize *offsets
)
{
	vkCmdBindVertexBuffers(
		m_buffer,
		firstBinding,
		count,
		buffers,
		offsets
	);
}

void CommandBuffer::bindIndexBuffer(
	VkBuffer buffer,
	VkDeviceSize offset,
	VkIndexType indexType
)
{
	vkCmdBindIndexBuffer(
		m_buffer,
		buffer,
		offset,
		indexType
	);
}

void CommandBuffer::pipelineBarrier(
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkDependencyFlags dependencyFlags,
	uint32_t memoryBarrierCount, const VkMemoryBarrier *memoryBarriers,
	uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *bufferMemoryBarriers,
	uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *imageMemoryBarriers
)
{
	vkCmdPipelineBarrier(
		m_buffer,
		srcStageMask,
		dstStageMask,
		dependencyFlags,
		memoryBarrierCount,			memoryBarriers,
		bufferMemoryBarrierCount,	bufferMemoryBarriers,
		imageMemoryBarrierCount,	imageMemoryBarriers
	);
}

void CommandBuffer::blitImage(
	VkImage srcImage, VkImageLayout srcImageLayout,
	VkImage dstImage, VkImageLayout dstImageLayout,
	uint32_t regionCount,
	const VkImageBlit *regions,
	VkFilter filter
)
{
	vkCmdBlitImage(
		m_buffer,
		srcImage, srcImageLayout,
		dstImage, dstImageLayout,
		regionCount,
		regions,
		filter
	);
}

void CommandBuffer::copyBufferToBuffer(
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	uint32_t regionCount,
	const VkBufferCopy *regions
)
{
	vkCmdCopyBuffer(
		m_buffer,
		srcBuffer,
		dstBuffer,
		regionCount,
		regions
	);
}

void CommandBuffer::copyBufferToImage(
	VkBuffer srcBuffer,
	VkImage dstImage,
	VkImageLayout dstImageLayout,
	uint32_t regionCount,
	const VkBufferImageCopy *regions
)
{
	vkCmdCopyBufferToImage(
		m_buffer,
		srcBuffer,
		dstImage,
		dstImageLayout,
		regionCount,
		regions
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

void CommandBuffer::dispatch(uint32_t gcX, uint32_t gcY, uint32_t gcZ)
{
	vkCmdDispatch(m_buffer, gcX, gcY, gcZ);
}

VkCommandBuffer CommandBuffer::getBuffer() const
{
	return m_buffer;
}
