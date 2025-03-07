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
	, m_currentPipelineLayout(VK_NULL_HANDLE)
	, m_viewport()
	, m_scissor()
	, m_currentTarget(nullptr)
	, m_currentRenderInfo()
{
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::setShader(ShaderEffect *shader)
{
	m_currentPipelineLayout = shader->getPipelineLayout();
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

	if (m_currentTarget)
	{
		if (m_currentTarget->getType() == RENDER_TARGET_TYPE_BACKBUFFER)
		{
			blitSemaphore = ((Backbuffer *)m_currentTarget)->getImageAvailableSemaphore();
			waitSemaphoreCount++;

			signalSemaphoreCount++;
			queueFinishedSemaphores[0] = ((Backbuffer *)m_currentTarget)->getRenderFinishedSemaphore();
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

	m_currentTarget = nullptr;
}

void CommandBuffer::beginRendering(GenericRenderTarget *target)
{
	m_currentTarget = target;

	beginRendering(m_currentTarget->getRenderInfo());
}

void CommandBuffer::beginRendering(const RenderInfo &info)
{
	m_currentRenderInfo = info;

	_beginRecording();

	if (m_currentTarget)
		m_currentTarget->beginRendering(*this);

	VkRenderingInfo renderInfo = info.getInfo();

	m_viewport.x = 0.0f;
	m_viewport.y = (float)info.getHeight();
	m_viewport.width = (float)info.getWidth();
	m_viewport.height = -(float)info.getHeight();
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { info.getWidth(), info.getHeight() };

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

	if (m_currentTarget)
		m_currentTarget->endRendering(*this);
}

const RenderInfo &CommandBuffer::getCurrentRenderInfo() const
{
	return m_currentRenderInfo;
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

void CommandBuffer::drawIndexed(
	uint32_t indexCount,
	uint32_t instanceCount,
	uint32_t firstIndex,
	int32_t vertexOffset,
	uint32_t firstInstance
)
{
	vkCmdSetViewport(m_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(m_buffer, 0, 1, &m_scissor);

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
	uint32_t firstSet,
	uint32_t setCount,
	const VkDescriptorSet *descriptorSets,
	uint32_t dynamicOffsetCount,
	const uint32_t *dynamicOffsets
)
{
	vkCmdBindDescriptorSets(
		m_buffer,
		m_currentTarget ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		m_currentPipelineLayout,
		firstSet,
		setCount, descriptorSets,
		dynamicOffsetCount,
		dynamicOffsets
	);
}

void CommandBuffer::setViewport(const VkViewport &viewport)
{
	m_viewport.x = viewport.x;
	m_viewport.y = viewport.height - viewport.y;
	m_viewport.width = viewport.width;
	m_viewport.height = -viewport.height;
}

void CommandBuffer::setScissor(const VkRect2D &scissor)
{
	m_scissor = scissor;
}

void CommandBuffer::pushConstants(
	VkShaderStageFlagBits stageFlags,
	uint32_t offset,
	uint32_t size,
	void *data
)
{
	vkCmdPushConstants(
		m_buffer,
		m_currentPipelineLayout,
		stageFlags,
		offset,
		size,
		data
	);
}

void CommandBuffer::pushConstants(
	VkShaderStageFlagBits stageFlags,
	ShaderParameters &params
)
{
	cauto &data = params.getPackedConstants();

	vkCmdPushConstants(
		m_buffer,
		m_currentPipelineLayout,
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
