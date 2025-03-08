#include "render_info.h"
#include "backend.h"
#include "util.h"
#include "backbuffer.h"

using namespace llt;

RenderInfo::RenderInfo()
	: m_colourAttachments()
	, m_depthAttachment()
	, m_colourFormats()
	, m_attachmentCount(0)
	, m_width(0)
	, m_height(0)
	, m_blendConstants{0.0f, 0.0f, 0.0f, 0.0f}
	, m_colourBlendAttachmentStates()
	, m_blendStateLogicOpEnabled(false)
	, m_blendStateLogicOp(VK_LOGIC_OP_COPY)
	, m_samples(VK_SAMPLE_COUNT_1_BIT)
{
}

RenderInfo::~RenderInfo()
{
	clear();
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

void RenderInfo::addColourAttachment(VkAttachmentLoadOp loadOp, VkImageView imageView, VkFormat format, VkImageView resolveView)
{
	VkRenderingAttachmentInfoKHR attachment = {};
	attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.imageView = imageView;
	attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.loadOp = loadOp;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };

	if (resolveView != VK_NULL_HANDLE)
	{
		attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.resolveImageView = resolveView;
		attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	}
	else
	{
		attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.resolveImageView = VK_NULL_HANDLE;
		attachment.resolveMode = VK_RESOLVE_MODE_NONE;
	}

	m_colourAttachments.pushBack(attachment);
	m_colourFormats.pushBack(format);

	m_colourBlendAttachmentStates.emplaceBack();
	m_colourBlendAttachmentStates.back().colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_colourBlendAttachmentStates.back().blendEnable = VK_TRUE;
	m_colourBlendAttachmentStates.back().srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_colourBlendAttachmentStates.back().dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_colourBlendAttachmentStates.back().colorBlendOp = VK_BLEND_OP_ADD;
	m_colourBlendAttachmentStates.back().srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colourBlendAttachmentStates.back().dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_colourBlendAttachmentStates.back().alphaBlendOp = VK_BLEND_OP_ADD;

	m_attachmentCount++;
}

void RenderInfo::addDepthAttachment(VkAttachmentLoadOp loadOp, VkImageView depthView, VkImageView resolveView)
{
	m_depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	m_depthAttachment.imageView = depthView;
	m_depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_depthAttachment.loadOp = loadOp;
	m_depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_depthAttachment.clearValue = { { 1.0f, 0 } };

	if (resolveView != VK_NULL_HANDLE)
	{
		m_depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		m_depthAttachment.resolveImageView = resolveView;
		m_depthAttachment.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	}
	else
	{
		m_depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		m_depthAttachment.resolveImageView = VK_NULL_HANDLE;
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

void RenderInfo::clear()
{
	m_colourAttachments.clear();
	m_depthAttachment = {};
	m_attachmentCount = 0;
}

void RenderInfo::setClearColour(int idx, VkClearValue value)
{
	m_colourAttachments[idx].clearValue = value;
}

void RenderInfo::setClearDepth(VkClearValue value)
{
	m_depthAttachment.clearValue = value;
}

void RenderInfo::setBlendState(const BlendState &state)
{
	m_blendConstants[0] = state.blendConstants[0];
	m_blendConstants[1] = state.blendConstants[1];
	m_blendConstants[2] = state.blendConstants[2];
	m_blendConstants[3] = state.blendConstants[3];

	m_blendStateLogicOpEnabled = state.blendOpEnabled;
	m_blendStateLogicOp = state.blendOp;

	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].colorWriteMask = 0;

		if (state.writeMask[0]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (state.writeMask[1]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (state.writeMask[2]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (state.writeMask[3]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

		m_colourBlendAttachmentStates[i].colorBlendOp = state.colour.op;
		m_colourBlendAttachmentStates[i].srcColorBlendFactor = state.colour.src;
		m_colourBlendAttachmentStates[i].dstColorBlendFactor = state.colour.dst;

		m_colourBlendAttachmentStates[i].alphaBlendOp = state.alpha.op;
		m_colourBlendAttachmentStates[i].srcAlphaBlendFactor = state.alpha.src;
		m_colourBlendAttachmentStates[i].dstAlphaBlendFactor = state.alpha.dst;

		m_colourBlendAttachmentStates[i].blendEnable = state.enabled ? VK_TRUE : VK_FALSE;
	}
}

const Vector<VkPipelineColorBlendAttachmentState>& RenderInfo::getColourBlendAttachmentStates() const
{
	return m_colourBlendAttachmentStates;
}

bool RenderInfo::isBlendStateLogicOpEnabled() const
{
	return m_blendStateLogicOpEnabled;
}

VkLogicOp RenderInfo::getBlendStateLogicOp() const
{
	return m_blendStateLogicOp;
}

const Array<float, 4>& RenderInfo::getBlendConstants() const
{
	return m_blendConstants;
}

float RenderInfo::getBlendConstant(int idx) const
{
	return m_blendConstants[idx];
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

const Vector<VkFormat>& RenderInfo::getColourAttachmentFormats() const
{
	return m_colourFormats;
}

const VkFormat& RenderInfo::getDepthAttachmentFormat() const
{
	return (m_depthAttachment.imageView != VK_NULL_HANDLE) ? vkutil::findDepthFormat(g_vulkanBackend->m_physicalData.device) : VK_FORMAT_UNDEFINED;
}

uint32_t RenderInfo::getWidth() const
{
	return m_width;
}

uint32_t RenderInfo::getHeight() const
{
	return m_height;
}
