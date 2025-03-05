#ifndef COMMAND_BUFFER_H_
#define COMMAND_BUFFER_H_

#define VK_NO_PROTOTYPES
#include "third_party/volk.h"

namespace llt
{
	class GenericRenderTarget;
	class RenderInfo;
	class ShaderParameters;
	class Pipeline;

	class CommandBuffer
	{
	public:
		static CommandBuffer fromGraphics();
		static CommandBuffer fromCompute();
		static CommandBuffer fromTransfer();

		CommandBuffer(VkCommandBuffer buffer);
		~CommandBuffer();

		void submit(VkSemaphore computeSemaphore = VK_NULL_HANDLE);

		void beginRendering(GenericRenderTarget *target);
		void beginRendering(const RenderInfo &info);
		
		void endRendering();

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
			VkPipelineBindPoint bindPoint,
			VkPipelineLayout pipelineLayout,
			uint32_t firstSet,
			uint32_t setCount,
			const VkDescriptorSet *descriptorSets,
			uint32_t dynamicOffsetCount,
			const uint32_t *dynamicOffsets
		);

		void bindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
		void bindPipeline(Pipeline &pipeline);

		void setViewports(
			uint32_t viewportCount,
			VkViewport *viewports,
			uint32_t firstViewport = 0
		);

		void setViewport(const VkViewport &viewport);

		void setViewport(const RenderInfo &info);

		void setScissors(
			uint32_t scissorCount,
			VkRect2D *scissors,
			uint32_t firstScissor = 0
		);

		void setScissor(const VkRect2D &scissor);

		void setScissor(const RenderInfo &info);

		void pushConstants(
			VkPipelineLayout pipelineLayout,
			VkShaderStageFlagBits stageFlags,
			uint32_t offset,
			uint32_t size,
			void *data
		);

		void pushConstants(
			VkPipelineLayout pipelineLayout,
			VkShaderStageFlagBits stageFlags,
			ShaderParameters &params
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
			uint32_t memoryBarrierCount,		const VkMemoryBarrier *memoryBarriers,
			uint32_t bufferMemoryBarrierCount,	const VkBufferMemoryBarrier *bufferMemoryBarriers,
			uint32_t imageMemoryBarrierCount,	const VkImageMemoryBarrier *imageMemoryBarriers
		);

		void blitImage(
			VkImage srcImage, VkImageLayout srcImageLayout,
			VkImage dstImage, VkImageLayout dstImageLayout,
			uint32_t regionCount,
			const VkImageBlit *regions,
			VkFilter filter
		);

		void copyBufferToBuffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			uint32_t regionCount,
			const VkBufferCopy *regions
		);

		void copyBufferToImage(
			VkBuffer srcBuffer,
			VkImage dstImage,
			VkImageLayout dstImageLayout,
			uint32_t regionCount,
			const VkBufferImageCopy *regions
		);

		// ---

		void beginCompute();
		void endCompute(VkSemaphore signalSemaphore);

		void dispatch(uint32_t gcX, uint32_t gcY, uint32_t gcZ);

		VkCommandBuffer getBuffer() const;

	private:
		void _beginRecording();

		VkCommandBuffer m_buffer;

		GenericRenderTarget *m_target;
	};
}

#endif // COMMAND_BUFFER_H_
