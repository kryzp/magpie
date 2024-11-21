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

void RenderTarget::cleanUp()
{
	m_renderPassBuilder.cleanUp();
	m_depth.cleanUp();

	for (Texture* texture : m_attachments)
	{
		if (texture && texture->getParent() == this) {
			delete texture;
		}
	}

	m_attachments.clear();
}

void RenderTarget::create()
{
	m_renderPassBuilder.setDimensions(m_width, m_height);

	int nColourAttachments = m_renderPassBuilder.getColourAttachmentCount();

	createDepthResources(nColourAttachments);

	Vector<VkClearValue> values(nColourAttachments + 1);
	m_renderPassBuilder.setClearValues(values);

	for (int i = 0; i < nColourAttachments; i++) {
		setClearColour(i, Colour::black());
	}

	setDepthStencilClear(1.0f, 0);

	Vector<VkImageView> attachments;

	for (int i = 0; i < nColourAttachments; i++) {
		attachments.pushBack(m_attachments[i]->getImageView());
	}
	
	attachments.pushBack(m_depth.getImageView());

	m_renderPassBuilder.createRenderPass(
		1,
		attachments.size(), attachments.data(),
		nullptr, 0
	);
}

void RenderTarget::createOnlyDepth()
{
	m_renderPassBuilder.setDimensions(m_width, m_height);

	createDepthResources(0);

	Vector<VkClearValue> values(1);
	m_renderPassBuilder.setClearValues(values);

	setDepthStencilClear(1.0f, 0);

	VkImageView attachments[] = {
		m_depth.getImageView()
	};

	m_renderPassBuilder.createRenderPass(
		1,
		LLT_ARRAY_LENGTH(attachments), attachments,
		nullptr, 0
	);
}

void RenderTarget::createDepthResources(int idx)
{
	VkFormat format = vkutil::findDepthFormat(g_vulkanBackend->physicalData.device);

	m_depth.setSize(m_width, m_height);
	m_depth.setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
	m_depth.setSampleCount(getMSAA());

	m_depth.flagAsDepthTexture();

	m_depth.createInternalResources();

	m_depth.transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	m_depth.setParent(this);

	m_renderPassBuilder.createDepthAttachment(idx, getMSAA());
}

void RenderTarget::setClearColour(int idx, const Colour& colour)
{
	LLT_ASSERT(m_renderPassBuilder.getColourAttachmentCount() > 0, "[VULKAN:RENDERTARGET] Must have at least one colour attachment!");

	VkClearValue value = {};
	colour.getPremultiplied().exportToFloat(value.color.float32);
	m_renderPassBuilder.setClearColour(idx, value);
}

void RenderTarget::setDepthStencilClear(float depth, uint32_t stencil)
{
	VkClearValue value = {};
	value.depthStencil = { depth, stencil };
	m_renderPassBuilder.setClearColour(m_renderPassBuilder.getColourAttachmentCount(), value);
}

const Texture* RenderTarget::getAttachment(int idx) const
{
	return m_attachments[idx];
}

const Texture* RenderTarget::getDepthAttachment() const
{
	return &m_depth;
}

void RenderTarget::setAttachment(int idx, Texture* texture)
{
	m_attachments[idx] = texture;

    m_renderPassBuilder.createColourAttachment(
		idx,
		texture->getInfo().format,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		false
	);
}

VkSampleCountFlagBits RenderTarget::getMSAA() const
{
	return VK_SAMPLE_COUNT_1_BIT;
}
