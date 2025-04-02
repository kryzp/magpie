#pragma once

#include <vector>

#include "third_party/volk.h"

namespace mgp
{
	class ImageView;
	class VulkanCore;

	class RenderInfo
	{
	public:
		RenderInfo(const VulkanCore *core);
		~RenderInfo();

		void clear();

		void addColourAttachment(VkAttachmentLoadOp loadOp, const ImageView &view);
		void addColourAttachmentWithResolve(VkAttachmentLoadOp loadOp, const ImageView &view, const VkImageView &resolve);

		void addDepthAttachment(VkAttachmentLoadOp loadOp, const ImageView &view);
		void addDepthAttachmentWithResolve(VkAttachmentLoadOp loadOp, const ImageView &view, const VkImageView &resolve);

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

		const std::vector<VkFormat> &getColourAttachmentFormats() const;
		VkFormat getDepthAttachmentFormat() const;

		uint32_t getWidth() const;
		uint32_t getHeight() const;

	private:
		uint32_t m_width;
		uint32_t m_height;

		const VulkanCore *m_core;

		std::vector<VkRenderingAttachmentInfoKHR> m_colourAttachments;
		VkRenderingAttachmentInfoKHR m_depthAttachment;

		std::vector<VkFormat> m_colourFormats;

		int m_attachmentCount;

		VkSampleCountFlagBits m_samples;
	};
}
