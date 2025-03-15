#ifndef VK_RENDER_PASS_BUILDER_H_
#define VK_RENDER_PASS_BUILDER_H_

#include "third_party/volk.h"

#include "container/array.h"
#include "container/vector.h"

#include "blend.h"

namespace llt
{
	class Texture;
	class VulkanCore;
	class Swapchain;

	/*
	 * Responsible for building up the final vulkan render pass which is needed when starting to render.
	 */
	class RenderInfo
	{
	public:
		RenderInfo();
		~RenderInfo();

		void clear();

		void addColourAttachment(VkAttachmentLoadOp loadOp, VkImageView imageView, VkFormat format, VkImageView resolveView = VK_NULL_HANDLE);
		void addDepthAttachment(VkAttachmentLoadOp loadOp, VkImageView depthView, VkImageView resolveView = VK_NULL_HANDLE);

		VkRenderingAttachmentInfoKHR &getColourAttachment(int idx);
		VkRenderingAttachmentInfoKHR &getDepthAttachment();

		VkRenderingInfo getInfo() const;

		void setClearColour(int idx, VkClearValue value);
		void setClearDepth(VkClearValue value);

		void setSize(uint32_t width, uint32_t height);

		VkSampleCountFlagBits getMSAA() const;
		void setMSAA(VkSampleCountFlagBits samples);

		int getColourAttachmentCount() const;
		int getAttachmentCount() const;

		const Vector<VkFormat> &getColourAttachmentFormats() const;
		const VkFormat &getDepthAttachmentFormat() const;

		uint32_t getWidth() const;
		uint32_t getHeight() const;

	private:
		uint32_t m_width;
		uint32_t m_height;

		Vector<VkRenderingAttachmentInfoKHR> m_colourAttachments;
		VkRenderingAttachmentInfoKHR m_depthAttachment;

		Vector<VkFormat> m_colourFormats;

		int m_attachmentCount;

		VkSampleCountFlagBits m_samples;
	};
}

#endif // VK_RENDER_PASS_BUILDER_H_
