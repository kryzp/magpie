#include "render_target.h"

#include "core.h"
#include "util.h"

#include "math/colour.h"

using namespace llt;

RenderTarget::RenderTarget(uint32_t width, uint32_t height)
	: GenericRenderTarget(width, height)
	, m_attachments()
	, m_resolveAttachments()
	, m_layoutQueue()
	, m_depth()
	, m_resolveDepth()
{
	m_type = RENDER_TARGET_TYPE_TEXTURE;
	m_renderInfo.setSize(m_width, m_height);
}

RenderTarget::~RenderTarget()
{
	cleanUp();
}

void RenderTarget::cleanUp()
{
	m_renderInfo.clear();

	for (Texture *texture : m_attachments)
	{
		if (texture->getParent() == this)
			delete texture;
	}

	m_attachments.clear();

	for (Texture *texture : m_resolveAttachments)
	{
		if (texture->getParent() == this)
			delete texture;
	}

	m_resolveAttachments.clear();

	if (m_depth && m_depth->getParent() == this)
		delete m_depth;

	if (m_resolveDepth)
		delete m_resolveDepth;
}

void RenderTarget::beginRendering(CommandBuffer &cmd)
{
	for (int i = 0; i < m_attachments.size(); i++)
	{
		Texture *col = getAttachment(i);

		if (!col->isTransient())
		{
			m_layoutQueue.push_back(col->getImageLayout());
			col->transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}
	}

	if (getDepthAttachment())
		getDepthAttachment()->transitionLayout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void RenderTarget::endRendering(CommandBuffer &cmd)
{
	for (int i = 0; i < m_attachments.size(); i++)
	{
		Texture *col = getAttachment(i);

		if (!col->isTransient())
		{
			VkImageLayout layout = m_layoutQueue.front();

			m_layoutQueue.pop_front();
			
			col->transitionLayout(cmd, layout);
		}
	}

	if (getDepthAttachment())
		getDepthAttachment()->transitionLayout(cmd, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
}

void RenderTarget::setClearColour(int idx, const Colour &colour)
{
	LLT_ASSERT(m_renderInfo.getColourAttachmentCount() > 0, "Must have at least one colour attachment!");

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

Texture *RenderTarget::getAttachment(int idx)
{
	return m_renderInfo.getMSAA() != VK_SAMPLE_COUNT_1_BIT ? m_resolveAttachments[idx] : m_attachments[idx];
}

Texture *RenderTarget::getDepthAttachment()
{
	return m_renderInfo.getMSAA() != VK_SAMPLE_COUNT_1_BIT ? m_resolveDepth : m_depth;
}

void RenderTarget::addAttachment(Texture *texture, int layer, int mip)
{
	if (!texture)
		return;

	m_attachments.pushBack(texture);

	VkImageView resolveView = VK_NULL_HANDLE;

	// todo: 99% certain that this won't work for layer != 0, just as im coding this :)!
	if (texture->getNumSamples() != VK_SAMPLE_COUNT_1_BIT)
	{
		Texture *resolve = new Texture();

		resolve->setSize(texture->getWidth(), texture->getHeight());
		resolve->setProperties(texture->getFormat(), texture->getTiling(), texture->getType());
		resolve->setSampleCount(VK_SAMPLE_COUNT_1_BIT);

		resolve->createInternalResources();

		resolve->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		resolve->setParent(this);

		m_resolveAttachments.pushBack(resolve);
		resolveView = resolve->getStandardView();
	}

	VkImageView view = texture->getView(1, layer, mip);

	m_renderInfo.addColourAttachment(
		VK_ATTACHMENT_LOAD_OP_LOAD,
		view,
		texture->getFormat(),
		resolveView
	);
}

void RenderTarget::setDepthAttachment(Texture *texture)
{
	if (m_depth)
	{
		m_renderInfo.getDepthAttachment().imageView = texture->getStandardView();
	}
	else
	{
		m_depth = texture;

		m_renderInfo.addDepthAttachment(
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			m_depth->getStandardView(),
			VK_NULL_HANDLE
		);

		setDepthStencilClear(1.0f, 0);
	}
}

void RenderTarget::createDepthAttachment()
{
	VkFormat format = vkutil::findDepthFormat(g_vkCore->m_physicalData.device);

	m_depth = new Texture();

	m_depth->setSize(m_width, m_height);
	m_depth->setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
	m_depth->setSampleCount(getMSAA());
	m_depth->flagAsDepthTexture();
	m_depth->createInternalResources();
	m_depth->transitionLayoutSingle(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	m_depth->setParent(this);

	VkImageView resolveView = VK_NULL_HANDLE;

	if (m_renderInfo.getMSAA() != VK_SAMPLE_COUNT_1_BIT)
	{
		m_resolveDepth = new Texture();

		m_resolveDepth->setSize(m_width, m_height);
		m_resolveDepth->setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
		m_resolveDepth->setSampleCount(VK_SAMPLE_COUNT_1_BIT);
		m_resolveDepth->flagAsDepthTexture();
		m_resolveDepth->createInternalResources();
		m_resolveDepth->transitionLayoutSingle(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		m_resolveDepth->setParent(this);

		resolveView = m_resolveDepth->getStandardView();
	}

	m_renderInfo.addDepthAttachment(
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		m_depth->getStandardView(),
		resolveView
	);

	setDepthStencilClear(1.0f, 0);
}

VkSampleCountFlagBits RenderTarget::getMSAA() const
{
	return m_renderInfo.getMSAA();
}

void RenderTarget::setMSAA(VkSampleCountFlagBits samples)
{
	m_renderInfo.setMSAA(samples);
}
