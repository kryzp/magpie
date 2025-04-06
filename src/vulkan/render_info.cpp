#include "render_info.h"

#include "core.h"
#include "toolbox.h"
#include "image.h"
#include "image_view.h"

using namespace mgp;

RenderInfo::RenderInfo(const VulkanCore *core)
	: m_colourAttachments()
	, m_depthAttachment()
	, m_colourFormats()
	, m_attachmentCount(0)
	, m_width(0)
	, m_height(0)
	, m_samples(VK_SAMPLE_COUNT_1_BIT)
	, m_core(core)
{
}

RenderInfo::~RenderInfo()
{
}

VkRenderingInfo RenderInfo::getInfo() const
{
	VkRenderingInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	info.renderArea.offset = { 0, 0 };
	info.renderArea.extent = { m_width, m_height };
	info.layerCount = 1;
	info.colorAttachmentCount = m_colourAttachments.size();
	info.pColorAttachments = m_colourAttachments.data();
	info.pDepthAttachment = (m_depthAttachment.imageView != VK_NULL_HANDLE) ? &m_depthAttachment : nullptr;
	info.pStencilAttachment = nullptr; // TODO: IMPLEMENT STENCIL RENDERING!!

	return info;
}

void RenderInfo::addColourAttachment(VkAttachmentLoadOp loadOp, const ImageView &view, const ImageView *resolve)
{
	VkRenderingAttachmentInfoKHR attachment = {};
	attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.imageView = view.getHandle();
	attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.loadOp = loadOp;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	if (resolve)
	{
		attachment.resolveImageView = resolve->getHandle();
		attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	}
	else
	{
		attachment.resolveImageView = VK_NULL_HANDLE;
		attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.resolveMode = VK_RESOLVE_MODE_NONE;
	}

	m_colourAttachments.push_back(attachment);
	m_colourFormats.push_back(view.getInfo().getFormat());

	m_attachmentCount++;
}

void RenderInfo::addDepthAttachment(VkAttachmentLoadOp loadOp, const ImageView &view, const ImageView *resolve)
{
	m_depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	m_depthAttachment.imageView = view.getHandle();
	m_depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_depthAttachment.loadOp = loadOp;
	m_depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_depthAttachment.clearValue = { { 1.0f, 0 } };

	if (resolve)
	{
		m_depthAttachment.resolveImageView = resolve->getHandle();
		m_depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_depthAttachment.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	}
	else
	{
		m_depthAttachment.resolveImageView = VK_NULL_HANDLE;
		m_depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
	}

	m_attachmentCount++;
}

VkRenderingAttachmentInfoKHR &RenderInfo::getColourAttachment(int idx)
{
	return m_colourAttachments[idx];
}

VkRenderingAttachmentInfoKHR &RenderInfo::getDepthAttachment()
{
	return m_depthAttachment;
}

void RenderInfo::setClearColour(int idx, VkClearValue value)
{
	m_colourAttachments[idx].clearValue = value;
}

void RenderInfo::setClearDepth(VkClearValue value)
{
	m_depthAttachment.clearValue = value;
}

void RenderInfo::setSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

VkSampleCountFlagBits RenderInfo::getMSAA() const
{
	return m_samples;
}

void RenderInfo::setMSAA(VkSampleCountFlagBits samples)
{
	m_samples = samples;
}

int RenderInfo::getColourAttachmentCount() const
{
	return m_colourAttachments.size();
}

int RenderInfo::getAttachmentCount() const
{
	return m_attachmentCount;
}

const std::vector<VkFormat> &RenderInfo::getColourAttachmentFormats() const
{
	return m_colourFormats;
}

VkFormat RenderInfo::getDepthAttachmentFormat() const
{
	return (m_depthAttachment.imageView != VK_NULL_HANDLE) ? vk_toolbox::findDepthFormat(m_core->getPhysicalDevice()) : VK_FORMAT_UNDEFINED;
}

uint32_t RenderInfo::getWidth() const
{
	return m_width;
}

uint32_t RenderInfo::getHeight() const
{
	return m_height;
}
