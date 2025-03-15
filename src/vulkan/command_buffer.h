#ifndef COMMAND_BUFFER_H_
#define COMMAND_BUFFER_H_

#define VK_NO_PROTOTYPES
#include "third_party/volk.h"

#include "render_info.h"

#include "container/vector.h"

namespace llt
{
	class GenericRenderTarget;
	class ShaderEffect;

	class CommandBuffer
	{
	public:
		static CommandBuffer fromGraphics();
		static CommandBuffer fromCompute();
		static CommandBuffer fromTransfer();

		CommandBuffer(VkCommandBuffer buffer);
		~CommandBuffer();

		void beginRecording();
		void submit(VkSemaphore computeSemaphore = VK_NULL_HANDLE);

		void beginRendering(GenericRenderTarget *target);
		void beginRendering(const RenderInfo &info);
		
		void endRendering();

		const RenderInfo &getCurrentRenderInfo() const;

		void bindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);

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
			VkPipelineLayout layout,
			const Vector<VkDescriptorSet> &descriptorSets,
			const Vector<uint32_t> &dynamicOffsets
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
			VkBuffer *buffers,
			VkDeviceSize *offsets
		);

		void bindIndexBuffer(
			VkBuffer buffer,
			VkDeviceSize offset,
			VkIndexType indexType
		);

		void pipelineBarrier(
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkDependencyFlags dependencyFlags,
			const Vector<VkMemoryBarrier> &memoryBarriers,
			const Vector<VkBufferMemoryBarrier> &bufferMemoryBarriers,
			const Vector<VkImageMemoryBarrier> &imageMemoryBarriers
		);

		void transitionForMipmapGeneration(Texture &texture);
		void generateMipmaps(const Texture &texture);

		void blitImage(
			VkImage srcImage, VkImageLayout srcImageLayout,
			VkImage dstImage, VkImageLayout dstImageLayout,
			const Vector<VkImageBlit> &regions,
			VkFilter filter
		);

		void copyBufferToBuffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			const Vector<VkBufferCopy> &regions
		);

		void copyBufferToImage(
			VkBuffer srcBuffer,
			VkImage dstImage,
			VkImageLayout dstImageLayout,
			const Vector<VkBufferImageCopy> &regions
		);

		void writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool pool, uint32_t query);

		void resetQueryPool(VkQueryPool pool, uint32_t firstQuery, uint32_t queryCount);

		void beginCompute();
		void endCompute(VkSemaphore signalSemaphore);

		void dispatch(uint32_t gcX, uint32_t gcY, uint32_t gcZ);

		VkCommandBuffer getHandle() const;

	private:
		VkCommandBuffer m_buffer;

		VkViewport m_viewport;
		VkRect2D m_scissor;

		GenericRenderTarget *m_currentTarget;
		RenderInfo m_currentRenderInfo;

		bool m_isRendering;
	};
}

#endif // COMMAND_BUFFER_H_
