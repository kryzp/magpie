#ifndef COMMAND_BUFFER_H_
#define COMMAND_BUFFER_H_

#define VK_NO_PROTOTYPES
#include "../third_party/volk.h"

namespace llt
{
	class GenericRenderTarget;
	class RenderInfo;

	class CommandBuffer
	{
	public:
		static CommandBuffer fromGraphics();
		static CommandBuffer fromCompute();
		static CommandBuffer fromTransfer();

		CommandBuffer(VkCommandBuffer buffer);
		~CommandBuffer();

		void submit(VkSemaphore computeSemaphore = VK_NULL_HANDLE);

		void beginRendering(GenericRenderTarget* target);
		void beginRendering(const RenderInfo& info);
		
		void endRendering();

		void beginCompute();
		void endCompute(VkSemaphore signalSemaphore);

		VkCommandBuffer getBuffer() const;

	private:
		void _beginRecording();

		VkCommandBuffer m_buffer;

		GenericRenderTarget* m_target;
	};
}

#endif // COMMAND_BUFFER_H_
