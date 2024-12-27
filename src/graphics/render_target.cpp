#include "render_target.h"
#include "backend.h"
#include "util.h"
#include "colour.h"

using namespace llt;

RenderTarget::RenderTarget()
	: GenericRenderTarget()
	, m_attachments()
	, m_resolveAttachments()
	, m_depth()
	, m_resolveDepth()
	, m_samples(VK_SAMPLE_COUNT_1_BIT)
{
	m_type = RENDER_TARGET_TYPE_TEXTURE;
}

RenderTarget::RenderTarget(uint32_t width, uint32_t height)
	: GenericRenderTarget(width, height)
	, m_attachments()
	, m_resolveAttachments()
	, m_depth()
	, m_resolveDepth()
	, m_samples(VK_SAMPLE_COUNT_1_BIT)
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

void RenderTarget::beginGraphics(VkCommandBuffer cmdBuffer)
{
	for (auto& col : (m_samples != VK_SAMPLE_COUNT_1_BIT) ? m_resolveAttachments : m_attachments)
	{
		if (!col->isTransient()) {
			col->transitionLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}
	}

	(m_samples != VK_SAMPLE_COUNT_1_BIT ? m_resolveDepth : m_depth).transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void RenderTarget::endGraphics(VkCommandBuffer cmdBuffer)
{
	for (auto& col : (m_samples != VK_SAMPLE_COUNT_1_BIT) ? m_resolveAttachments : m_attachments)
	{
		if (!col->isTransient()) {
			col->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	(m_samples != VK_SAMPLE_COUNT_1_BIT ? m_resolveDepth : m_depth).transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
}

void RenderTarget::cleanUp()
{
	m_renderInfo.clear();

	m_depth.cleanUp();
	m_resolveDepth.cleanUp();

	for (Texture* texture : m_attachments)
	{
		if (texture->getParent() == this) {
			delete texture;
		}
	}

	for (Texture* texture : m_resolveAttachments)
	{
		delete texture;
	}

	m_attachments.clear();
}

void RenderTarget::setClearColour(int idx, const Colour& colour)
{
	LLT_ASSERT(m_renderInfo.getColourAttachmentCount() > 0, "[RENDERTARGET] Must have at least one colour attachment!");

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

Texture* RenderTarget::getAttachment(int idx)
{
	return m_samples != VK_SAMPLE_COUNT_1_BIT ? m_resolveAttachments[idx] : m_attachments[idx];
}

Texture* RenderTarget::getDepthAttachment()
{
	return m_samples != VK_SAMPLE_COUNT_1_BIT ? &m_resolveDepth : &m_depth;
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

	if (m_samples != VK_SAMPLE_COUNT_1_BIT)
	{
		m_resolveDepth.setSize(m_width, m_height);
		m_resolveDepth.setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
		m_resolveDepth.setSampleCount(VK_SAMPLE_COUNT_1_BIT);
		m_resolveDepth.flagAsDepthTexture();
		m_resolveDepth.createInternalResources();
		m_resolveDepth.transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		m_resolveDepth.setParent(this);

		m_renderInfo.addDepthAttachment(
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			&m_depth,
			m_resolveDepth.getImageView()
		);
	}
	else
	{
		m_renderInfo.addDepthAttachment(
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			&m_depth,
			VK_NULL_HANDLE
		);
	}
}

void RenderTarget::addAttachment(Texture* texture)
{
	if (!texture)
		return;

	if (texture->getNumSamples() != VK_SAMPLE_COUNT_1_BIT)
	{
		Texture* resolve = new Texture();

		resolve->setSize(texture->getWidth(), texture->getHeight());
		resolve->setProperties(texture->getInfo().format, texture->getInfo().tiling, texture->getInfo().type);
		resolve->setSampleCount(VK_SAMPLE_COUNT_1_BIT);

		resolve->createInternalResources();

		resolve->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		resolve->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		resolve->setParent(this);

		m_attachments.pushBack(texture);
		m_resolveAttachments.pushBack(resolve);

		m_renderInfo.addColourAttachment(
			VK_ATTACHMENT_LOAD_OP_LOAD,
			texture->getImageView(),
			texture->getInfo().format,
			resolve->getImageView()
		);
	}
	else
	{
		m_attachments.pushBack(texture);

		m_renderInfo.addColourAttachment(
			VK_ATTACHMENT_LOAD_OP_LOAD,
			texture->getImageView(),
			texture->getInfo().format,
			VK_NULL_HANDLE
		);
	}
}

VkSampleCountFlagBits RenderTarget::getMSAA() const
{
	return m_samples;
}

void RenderTarget::setMSAA(VkSampleCountFlagBits samples)
{
	m_samples = samples;
}
