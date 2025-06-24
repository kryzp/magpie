#pragma once

#include <vector>

#include <Volk/volk.h>

#include "math/rect.h"

namespace mgp
{
	class Image;
	class GPUBuffer;
	class RenderInfo;
	class Descriptor;

	class CommandBuffer
	{
	public:
		CommandBuffer() = default;
		CommandBuffer(VkCommandBuffer buffer);

		~CommandBuffer();

		VkCommandBufferSubmitInfo getSubmitInfo() const;

		void begin();
		void end();

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

		void bindDescriptors(
			uint32_t first,
			VkPipelineBindPoint bindPoint,
			VkPipelineLayout layout,
			const std::vector<Descriptor *> &descriptors,
			const std::vector<uint32_t> &dynamicOffsets
		);

		void setViewport(const VkViewport &viewport);
		void setScissor(const VkRect2D &scissor);

		void pushConstants(
			VkPipelineLayout layout,
			VkShaderStageFlags stageFlags,
			uint32_t size,
			void *data,
			uint32_t offset = 0
		);

		void bindVertexBuffers(
			uint32_t firstBinding,
			uint32_t count,
			GPUBuffer *const *buffers,
			const VkDeviceSize *offsets
		);

		void bindIndexBuffer(
			const GPUBuffer *buffer,
			VkDeviceSize offset,
			VkIndexType indexType
		);

		void pipelineBarrier(
			VkDependencyFlags dependencyFlags,
			const std::vector<VkMemoryBarrier2> &memoryBarriers,
			const std::vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
			const std::vector<VkImageMemoryBarrier2> &imageMemoryBarriers
		);

		void transitionLayout(Image *image, VkImageLayout newLayout);
		void generateMipmaps(Image *image);

		void blitImage(
			VkImage srcImage, VkImageLayout srcImageLayout,
			VkImage dstImage, VkImageLayout dstImageLayout,
			const std::vector<VkImageBlit> &regions,
			VkFilter filter
		);

		void copyBufferToBuffer(
			const GPUBuffer *srcBuffer,
			const GPUBuffer *dstBuffer,
			const std::vector<VkBufferCopy> &regions
		);

		void copyBufferToImage(
			const GPUBuffer *buffer,
			const Image *image
		);

		void copyBufferToImage(
			const GPUBuffer *buffer,
			const Image *image,
			const std::vector<VkBufferImageCopy> &regions
		);

		void dispatch(
			uint32_t gcX,
			uint32_t gcY,
			uint32_t gcZ
		);

		void writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool pool, uint32_t query);
		void resetQueryPool(VkQueryPool pool, uint32_t firstQuery, uint32_t queryCount);

		VkCommandBuffer getHandle() const;

	private:
		VkCommandBuffer m_buffer;

		VkViewport m_viewport;
		VkRect2D m_scissor;
	};
}
