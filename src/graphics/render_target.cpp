#include "render_target.h"
#include "backend.h"
#include "util.h"
#include "colour.h"

using namespace llt;

RenderTarget::RenderTarget()
	: GenericRenderTarget()
	, m_depth()
{
	m_type = RENDER_TARGET_TYPE_TEXTURE;
}

RenderTarget::RenderTarget(uint32_t width, uint32_t height)
	: GenericRenderTarget(width, height)
	, m_depth()
{
	m_type = RENDER_TARGET_TYPE_TEXTURE;
}

RenderTarget::~RenderTarget()
{
	cleanUp();
}

void RenderTarget::create()
{
	m_renderInfo.setDimensions(m_width, m_height);

	setClearColours(Colour::black());

	createDepthResources();
	setDepthStencilClear(1.0f, 0);
}

void RenderTarget::createOnlyDepth()
{
	m_renderInfo.setDimensions(m_width, m_height);

	createDepthResources();

	setDepthStencilClear(1.0f, 0);
}

void RenderTarget::createDepthResources()
{
	VkFormat format = vkutil::findDepthFormat(g_vulkanBackend->physicalData.device);

	m_depth.setSize(m_width, m_height);
	m_depth.setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
	m_depth.setSampleCount(getMSAA());
	m_depth.flagAsDepthTexture();
	m_depth.createInternalResources();
	m_depth.transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	m_depth.setParent(this);

	m_renderInfo.addDepthAttachment(VK_ATTACHMENT_LOAD_OP_CLEAR, &m_depth);
}

void RenderTarget::beginGraphics(VkCommandBuffer cmdBuffer)
{
	for (auto& col : m_attachments)
	{
		if (!col->isTransient())
			col->transitionLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	}

	m_depth.transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void RenderTarget::endGraphics(VkCommandBuffer cmdBuffer)
{
	for (auto& col : m_attachments)
	{
		if (!col->isTransient())
			col->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	m_depth.transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
}

void RenderTarget::cleanUp()
{
	m_renderInfo.clear();
	m_depth.cleanUp();

	for (Texture* texture : m_attachments)
	{
		if (texture->getParent() == this) {
			delete texture;
		}
	}

	m_attachments.clear();
}

void RenderTarget::setClearColour(int idx, const Colour& colour)
{
	LLT_ASSERT(m_renderInfo.getColourAttachmentCount() > 0, "[VULKAN:RENDERTARGET] Must have at least one colour attachment!");

	VkClearValue value = {};
	colour.getPremultiplied().exportToFloat(value.color.float32);
	m_renderInfo.setClearColour(idx, value);
}

void RenderTarget::setDepthStencilClear(float depth, uint32_t stencil)
{
	VkClearValue value = {};
	value.depthStencil = { depth, stencil };
	m_renderInfo.setClearDepth(value);
}

const Texture* RenderTarget::getAttachment(int idx) const
{
	return m_attachments[idx];
}

const Texture* RenderTarget::getDepthAttachment() const
{
	return &m_depth;
}

void RenderTarget::addAttachment(Texture* texture)
{
	if (!texture)
		return;

	m_attachments.pushBack(texture);

    m_renderInfo.addColourAttachment(
		VK_ATTACHMENT_LOAD_OP_LOAD,
		texture->getImageView(),
		texture->getInfo().format,
		VK_NULL_HANDLE
	);
}

VkSampleCountFlagBits RenderTarget::getMSAA() const
{
	return VK_SAMPLE_COUNT_1_BIT;
}
