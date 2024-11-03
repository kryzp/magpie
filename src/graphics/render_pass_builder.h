#ifndef VK_RENDER_PASS_BUILDER_H_
#define VK_RENDER_PASS_BUILDER_H_

#include "../container/array.h"
#include "../container/vector.h"

#include <vulkan/vulkan.h>

namespace llt
{
	class VulkanBackend;
	class Backbuffer;

	/*
	 * Responsible for building up the final vulkan render pass which is needed when starting to render.
	 */
	class RenderPassBuilder
	{
	public:
		RenderPassBuilder();
		~RenderPassBuilder();

		void cleanUp();
		void cleanUpFramebuffers();

		void createRenderPass(uint64_t nFramebuffers, uint64_t attachmentCount, VkImageView* attachments, VkImageView* backbufferAttachments, uint64_t backbufferAttachmentIdx);

		void createColourAttachment(int idx, VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkImageLayout finalLayout, bool isResolve);
		void createDepthAttachment(int idx, VkSampleCountFlagBits samples);

		VkRenderPassBeginInfo buildBeginInfo() const;

		void makeBackbuffer(Backbuffer* backbuffer);

		void setClearColour(int idx, VkClearValue value);
		void setClearValues(const Vector<VkClearValue>& values);

		VkRenderPass getRenderPass();

		void setDimensions(uint32_t width, uint32_t height);

		int getColourAttachmentCount() const;
		int getAttachmentCount() const;

		uint32_t getWidth() const;
		uint32_t getHeight() const;

	private:
		void destroyRenderPass();
		void destroyFBOs();

		Backbuffer* m_backbuffer;

		//Queue* m_queue;

		uint32_t m_width;
		uint32_t m_height;

		Vector<VkClearValue> m_clearColours;
		VkRenderPass m_renderPass;

		Array<VkAttachmentDescription, mgc::MAX_RENDER_TARGET_ATTACHMENTS> m_attachmentDescriptions;

		Vector<VkAttachmentReference> m_colourAttachments;
		Vector<VkAttachmentReference> m_colourAttachmentResolves;
		VkAttachmentReference m_depthAttachmentRef;

		Vector<VkFramebuffer> m_framebuffers;

		int m_colourAttachmentCount;
		int m_attachmentCount;
	};
}

#endif // VK_RENDER_PASS_BUILDER_H_
