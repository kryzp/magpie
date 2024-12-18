#include "render_info_builder.h"
#include "backend.h"
#include "util.h"
#include "backbuffer.h"

using namespace llt;

RenderInfoBuilder::RenderInfoBuilder()
	: m_colourAttachments()
	, m_depthAttachment()
	, m_colourFormats()
	, m_attachmentCount(0)
	, m_width(0)
	, m_height(0)
{
}

RenderInfoBuilder::~RenderInfoBuilder()
{
	clear();
}

VkRenderingInfoKHR RenderInfoBuilder::buildInfo() const
{
	VkRenderingInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	info.renderArea.offset = { 0, 0 };
	info.renderArea.extent = { m_width, m_height };
	info.layerCount = 1;
	info.colorAttachmentCount = m_colourAttachments.size();
	info.pColorAttachments = m_colourAttachments.data();
	info.pDepthAttachment = &m_depthAttachment;
	info.pStencilAttachment = nullptr; // TODO: IMPLEMENT STENCIL RENDERING!!

	return info;
}

void RenderInfoBuilder::addColourAttachment(VkAttachmentLoadOp loadOp, VkImageView imageView, VkFormat format, VkImageView resolveView)
{
	VkRenderingAttachmentInfoKHR attachment = {};
	attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.imageView = imageView;
	attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.loadOp = loadOp;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } }; // default to black

	if (resolveView != VK_NULL_HANDLE)
	{
		attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.resolveImageView = resolveView;
		attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	}

	m_colourAttachments.pushBack(attachment);
	m_colourFormats.pushBack(format);

	m_attachmentCount++;
}

void RenderInfoBuilder::addDepthAttachment(VkAttachmentLoadOp loadOp, Texture* texture)
{
	m_depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	m_depthAttachment.imageView = texture->getImageView();
	m_depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_depthAttachment.loadOp = loadOp;
	m_depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_depthAttachment.clearValue = { { 1.0f, 0 } };

	m_attachmentCount++;
}

VkRenderingAttachmentInfoKHR& RenderInfoBuilder::getColourAttachment(int idx)
{
	return m_colourAttachments[idx];
}

VkRenderingAttachmentInfoKHR& RenderInfoBuilder::getDepthAttachment()
{
	return m_depthAttachment;
}

void RenderInfoBuilder::clear()
{
	m_colourAttachments.clear();
	m_depthAttachment = {};
	m_attachmentCount = 0;
}

void RenderInfoBuilder::setClearColour(int idx, VkClearValue value)
{
	m_colourAttachments[idx].clearValue = value;
}

void RenderInfoBuilder::setClearDepth(VkClearValue value)
{
	m_depthAttachment.clearValue = value;
}

void RenderInfoBuilder::setDimensions(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

int RenderInfoBuilder::getColourAttachmentCount() const
{
	return m_colourAttachments.size();
}

int RenderInfoBuilder::getAttachmentCount() const
{
	return m_attachmentCount;
}

const Vector<VkFormat>& RenderInfoBuilder::getColourAttachmentFormats() const
{
	return m_colourFormats;
}

uint32_t RenderInfoBuilder::getWidth() const
{
	return m_width;
}

uint32_t RenderInfoBuilder::getHeight() const
{
	return m_height;
}
