#pragma once

#include <vector>

#include <Volk/volk.h>

namespace mgp
{
	class ImageView;
	class VulkanCore;

	class RenderInfo
	{
	public:
		RenderInfo(const VulkanCore *core);
		~RenderInfo();

		void addColourAttachment(VkAttachmentLoadOp loadOp, const ImageView &view, const ImageView *resolve);
		void addDepthAttachment(VkAttachmentLoadOp loadOp, const ImageView &view, const ImageView *resolve);

		VkRenderingAttachmentInfo &getColourAttachment(int idx);
		VkRenderingAttachmentInfo &getDepthAttachment();

		VkRenderingInfo getInfo() const;

		void setClearColour(int idx, VkClearValue value);
		void setClearDepth(VkClearValue value);

		void setSize(uint32_t width, uint32_t height);

		VkSampleCountFlagBits getMSAA() const;
		void setMSAA(VkSampleCountFlagBits samples);

		int getColourAttachmentCount() const;
		int getAttachmentCount() const;

		const std::vector<VkFormat> &getColourAttachmentFormats() const;
		VkFormat getDepthAttachmentFormat() const;

		uint32_t getWidth() const;
		uint32_t getHeight() const;

	private:
		uint32_t m_width;
		uint32_t m_height;

		const VulkanCore *m_core;

		std::vector<VkRenderingAttachmentInfo> m_colourAttachments;
		VkRenderingAttachmentInfo m_depthAttachment;

		std::vector<VkFormat> m_colourFormats;

		int m_attachmentCount;

		VkSampleCountFlagBits m_samples;
	};
}
