#ifndef VK_RENDER_PASS_BUILDER_H_
#define VK_RENDER_PASS_BUILDER_H_

#include "../container/array.h"
#include "../container/vector.h"

#include <vulkan/vulkan.h>

namespace llt
{
	class Texture;
	class VulkanBackend;
	class Backbuffer;

	/*
	 * Responsible for building up the final vulkan render pass which is needed when starting to render.
	 */
	class RenderInfoBuilder
	{
	public:
		RenderInfoBuilder();
		~RenderInfoBuilder();

		void clear();

		void addColourAttachment(VkAttachmentLoadOp loadOp, VkImageView imageView, VkFormat format, VkImageView resolveView);
		void addDepthAttachment(VkAttachmentLoadOp loadOp, Texture* texture);

		VkRenderingAttachmentInfoKHR& getColourAttachment(int idx);
		VkRenderingAttachmentInfoKHR& getDepthAttachment();

		VkRenderingInfoKHR buildInfo() const;

		void setClearColour(int idx, VkClearValue value);
		void setClearDepth(VkClearValue value);

		void setDimensions(uint32_t width, uint32_t height);

		int getColourAttachmentCount() const;
		int getAttachmentCount() const;

		const Vector<VkFormat>& getColourAttachmentFormats() const;

		uint32_t getWidth() const;
		uint32_t getHeight() const;

	private:
		uint32_t m_width;
		uint32_t m_height;

		Vector<VkRenderingAttachmentInfoKHR> m_colourAttachments;
		VkRenderingAttachmentInfoKHR m_depthAttachment;

		Vector<VkFormat> m_colourFormats;

		int m_attachmentCount;
	};
}

#endif // VK_RENDER_PASS_BUILDER_H_
