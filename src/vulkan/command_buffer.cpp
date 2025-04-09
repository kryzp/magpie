#include "command_buffer.h"

#include "core/common.h"

#include "image.h"
#include "gpu_buffer.h"
#include "toolbox.h"

using namespace mgp;

CommandBuffer::CommandBuffer(VkCommandBuffer buffer)
	: m_buffer(buffer)
	, m_viewport()
	, m_scissor()
{
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::begin()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	MGP_VK_CHECK(
		vkBeginCommandBuffer(m_buffer, &commandBufferBeginInfo),
		"Failed to begin recording instant command buffer"
	);
}

void CommandBuffer::end()
{
	MGP_VK_CHECK(
		vkEndCommandBuffer(m_buffer),
		"Failed to record command buffer"
	);
}

VkCommandBufferSubmitInfo CommandBuffer::getSubmitInfo() const
{
	VkCommandBufferSubmitInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	bufferInfo.deviceMask = 0;
	bufferInfo.commandBuffer = m_buffer;

	return bufferInfo;
}

void CommandBuffer::beginRendering(const RenderInfo &target)
{
	VkRenderingInfo renderInfo = target.getInfo();

	m_viewport.x = 0.0f;
	m_viewport.y = (float)target.getHeight();
	m_viewport.width = (float)target.getWidth();
	m_viewport.height = -(float)target.getHeight();
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { target.getWidth(), target.getHeight() };

	vkCmdBeginRenderingKHR(m_buffer, &renderInfo);
}

void CommandBuffer::endRendering()
{
	vkCmdEndRenderingKHR(m_buffer);
}

void CommandBuffer::bindPipeline(
	VkPipelineBindPoint bindPoint,
	VkPipeline pipeline
)
{
	vkCmdBindPipeline(
		m_buffer,
		bindPoint,
		pipeline
	);
}

void CommandBuffer::draw(
	uint32_t vertexCount,
	uint32_t instanceCount,
	uint32_t firstVertex,
	uint32_t firstInstance
)
{
	vkCmdSetViewport(m_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(m_buffer, 0, 1, &m_scissor);

	vkCmdDraw(
		m_buffer,
		vertexCount,
		instanceCount,
		firstVertex,
		firstInstance
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
	vkCmdSetViewport(m_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(m_buffer, 0, 1, &m_scissor);

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
	vkCmdSetViewport(m_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(m_buffer, 0, 1, &m_scissor);

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
	VkPipelineBindPoint bindPoint,
	VkPipelineLayout layout,
	const std::vector<VkDescriptorSet> &descriptorSets,
	const std::vector<uint32_t> &dynamicOffsets
)
{
	vkCmdBindDescriptorSets(
		m_buffer,
		bindPoint,
		layout,
		firstSet,
		descriptorSets.size(),
		descriptorSets.data(),
		dynamicOffsets.size(),
		dynamicOffsets.data()
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
	VkPipelineLayout layout,
	VkShaderStageFlagBits stageFlags,
	uint32_t size,
	void *data,
	uint32_t offset
)
{
	vkCmdPushConstants(
		m_buffer,
		layout,
		stageFlags,
		offset,
		size,
		data
	);
}

void CommandBuffer::bindVertexBuffers(
	uint32_t firstBinding,
	uint32_t count,
	const VkBuffer *buffers,
	const VkDeviceSize *offsets
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
	VkDependencyFlags dependencyFlags,
	const std::vector<VkMemoryBarrier2> &memoryBarriers,
	const std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
	const std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers
)
{
	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;

	dependency.dependencyFlags = dependencyFlags;

	dependency.memoryBarrierCount = memoryBarriers.size();
	dependency.pMemoryBarriers = memoryBarriers.data();

	dependency.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
	dependency.pBufferMemoryBarriers =	bufferMemoryBarriers.data();

	dependency.imageMemoryBarrierCount = imageMemoryBarriers.size();
	dependency.pImageMemoryBarriers = imageMemoryBarriers.data();

	vkCmdPipelineBarrier2KHR(
		m_buffer,
		&dependency
	);
}

void CommandBuffer::transitionLayout(Image &image, VkImageLayout newLayout)
{
	VkImageMemoryBarrier2 barrier = image.getBarrier(newLayout);

	barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;//vkutil::getTransferAccessFlags(m_imageLayout);
	barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;//vkutil::getTransferAccessFlags(newLayout);

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;//vk_toolbox::getTransferPipelineStageFlags(image.getLayout());
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;//vk_toolbox::getTransferPipelineStageFlags(newLayout);

	pipelineBarrier(
		0,
		{}, {}, { barrier }
	);

	image.m_layout = newLayout;
}

void CommandBuffer::generateMipmaps(Image &image)
{
	MGP_ASSERT(image.getLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, "image must be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL");

	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = image.getHandle();
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = image.getLayerCount();
	barrier.subresourceRange.levelCount = 1;

	for (int i = 1; i < image.getMipmapCount(); i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

		pipelineBarrier(
			0,
			{}, {}, { barrier }
		);

		for (int face = 0; face < image.getFaceCount(); face++)
		{
			int srcMipWidth  = (int)image.getWidth()	>> (i - 1);
			int srcMipHeight = (int)image.getHeight()	>> (i - 1);
			int dstMipWidth  = (int)image.getWidth()	>> (i - 0);
			int dstMipHeight = (int)image.getHeight()	>> (i - 0);

			VkImageBlit blit = {};

			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { srcMipWidth, srcMipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { dstMipWidth, dstMipHeight, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;

			blitImage(
				image.getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image.getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				{ blit },
				VK_FILTER_LINEAR
			);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		pipelineBarrier(
			0,
			{}, {}, { barrier }
		);
	}

	barrier.subresourceRange.baseMipLevel = image.getMipmapCount() - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

	pipelineBarrier(
		0,
		{}, {}, { barrier }
	);

	image.m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void CommandBuffer::blitImage(
	VkImage srcImage, VkImageLayout srcImageLayout,
	VkImage dstImage, VkImageLayout dstImageLayout,
	const std::vector<VkImageBlit> &regions,
	VkFilter filter
)
{
	vkCmdBlitImage(
		m_buffer,
		srcImage, srcImageLayout,
		dstImage, dstImageLayout,
		regions.size(),
		regions.data(),
		filter
	);
}

void CommandBuffer::copyBufferToBuffer(
	const GPUBuffer &srcBuffer,
	const GPUBuffer &dstBuffer,
	const std::vector<VkBufferCopy> &regions
)
{
	copyBufferToBuffer(
		srcBuffer.getHandle(),
		dstBuffer.getHandle(),
		regions
	);
}

void CommandBuffer::copyBufferToBuffer(
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	const std::vector<VkBufferCopy> &regions
)
{
	vkCmdCopyBuffer(
		m_buffer,
		srcBuffer,
		dstBuffer,
		regions.size(),
		regions.data()
	);
}

void CommandBuffer::copyBufferToImage(
	const GPUBuffer &buffer,
	const Image &image
)
{
	MGP_ASSERT(image.getLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, "image must be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL");

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { image.getWidth(), image.getHeight(), 1 };

	copyBufferToImage(
		buffer.getHandle(),
		image.getHandle(),
		{ region }
	);
}

void CommandBuffer::copyBufferToImage(
	const GPUBuffer &buffer,
	const Image &image,
	const std::vector<VkBufferImageCopy> &regions
)
{
	MGP_ASSERT(image.getLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, "image must be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL");

	copyBufferToImage(
		buffer.getHandle(),
		image.getHandle(),
		regions
	);
}

void CommandBuffer::copyBufferToImage(
	VkBuffer srcBuffer,
	VkImage dstImage,
	const std::vector<VkBufferImageCopy> &regions
)
{
	vkCmdCopyBufferToImage(
		m_buffer,
		srcBuffer,
		dstImage,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		regions.size(),
		regions.data()
	);
}

void CommandBuffer::writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool pool, uint32_t query)
{
	vkCmdWriteTimestamp(
		m_buffer,
		pipelineStage,
		pool,
		query
	);
}

void CommandBuffer::resetQueryPool(VkQueryPool pool, uint32_t firstQuery, uint32_t queryCount)
{
	vkCmdResetQueryPool(
		m_buffer,
		pool,
		firstQuery,
		queryCount
	);
}

void CommandBuffer::dispatch(uint32_t gcX, uint32_t gcY, uint32_t gcZ)
{
	vkCmdDispatch(
		m_buffer,
		gcX, gcY, gcZ
	);
}

VkCommandBuffer CommandBuffer::getHandle() const
{
	return m_buffer;
}
