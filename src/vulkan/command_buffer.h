#pragma once

#include <vector>

#include "third_party/volk.h"

#include "render_info.h"

namespace mgp
{
	class Image;
	class ImageInfo;

	class CommandBuffer
	{
	public:
		CommandBuffer(VkCommandBuffer buffer);
		~CommandBuffer();

		void begin();
		void end();

		VkCommandBufferSubmitInfo getSubmitInfo() const;

		void beginRendering(const RenderInfo &info);
		void endRendering();

		void bindPipeline(
			VkPipelineBindPoint bindPoint,
			VkPipeline pipeline
		);

		void draw(
			uint32_t vertexCount,
			uint32_t instanceCount = 1,
			uint32_t firstVertex = 0,
			uint32_t firstInstance = 0
		);

		void drawIndexed(
			uint32_t indexCount,
			uint32_t instanceCount = 1,
			uint32_t firstIndex = 0,
			int32_t vertexOffset = 0,
			uint32_t firstInstance = 0
		);

		void drawIndexedIndirect(
			VkBuffer buffer,
			VkDeviceSize offset,
			uint32_t drawCount,
			uint32_t stride
		);

		void drawIndexedIndirectCount(
			VkBuffer buffer,
			VkDeviceSize offset,
			VkBuffer countBuffer,
			VkDeviceSize countBufferOffset,
			uint32_t maxDrawCount,
			uint32_t stride
		);

		void bindDescriptorSets(
			uint32_t firstSet,
			VkPipelineBindPoint bindPoint,
			VkPipelineLayout layout,
			const std::vector<VkDescriptorSet> &descriptorSets,
			const std::vector<uint32_t> &dynamicOffsets
		);

		void setViewport(const VkViewport &viewport);
		void setScissor(const VkRect2D &scissor);

		void pushConstants(
			VkPipelineLayout layout,
			VkShaderStageFlagBits stageFlags,
			uint32_t size,
			void *data,
			uint32_t offset = 0
		);

		void bindVertexBuffers(
			uint32_t firstBinding,
			uint32_t count,
			const VkBuffer *buffers,
			const VkDeviceSize *offsets
		);

		void bindIndexBuffer(
			VkBuffer buffer,
			VkDeviceSize offset,
			VkIndexType indexType
		);

		void pipelineBarrier(
			VkDependencyFlags dependencyFlags,
			const std::vector<VkMemoryBarrier2> &memoryBarriers,
			const std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
			const std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers
		);

		void transitionLayout(Image &image, VkImageLayout newLayout);
		void transitionLayout(ImageInfo &info, VkImageLayout newLayout);

		void generateMipmaps(Image &image);

		void blitImage(
			VkImage srcImage, VkImageLayout srcImageLayout,
			VkImage dstImage, VkImageLayout dstImageLayout,
			const std::vector<VkImageBlit> &regions,
			VkFilter filter
		);

		void copyBufferToBuffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			const std::vector<VkBufferCopy> &regions
		);

		void copyBufferToImage(
			VkBuffer srcBuffer,
			VkImage dstImage,
			VkImageLayout dstImageLayout,
			const std::vector<VkBufferImageCopy> &regions
		);

		void writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool pool, uint32_t query);

		void resetQueryPool(VkQueryPool pool, uint32_t firstQuery, uint32_t queryCount);

		void dispatch(uint32_t gcX, uint32_t gcY, uint32_t gcZ);

		VkCommandBuffer getHandle() const;

	private:
		VkCommandBuffer m_buffer;

		VkViewport m_viewport;
		VkRect2D m_scissor;
	};
}
