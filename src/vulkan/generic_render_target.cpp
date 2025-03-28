#include "generic_render_target.h"

#include "math/colour.h"

using namespace llt;

GenericRenderTarget::GenericRenderTarget()
	: m_type(RENDER_TARGET_TYPE_MAX_ENUM)
	, m_width(0)
	, m_height(0)
	, m_renderInfo()
{
}

GenericRenderTarget::GenericRenderTarget(uint32_t width, uint32_t height)
	: m_type(RENDER_TARGET_TYPE_MAX_ENUM)
	, m_width(width)
	, m_height(height)
	, m_renderInfo()
{
}

GenericRenderTarget::~GenericRenderTarget()
{
}

uint64_t GenericRenderTarget::getAttachmentCount() const
{
	return m_renderInfo.getColourAttachmentCount();
}

void GenericRenderTarget::setClearColours(const Colour &colour)
{
	for (int i = 0; i < getAttachmentCount(); i++)
	{
		setClearColour(i, colour);
	}
}

void GenericRenderTarget::toggleClear(bool clear)
{
	for (int i = 0; i < getAttachmentCount(); i++)
	{
		toggleColourClear(i, clear);
	}

	toggleDepthStencilClear(clear);
}

void GenericRenderTarget::toggleColourClear(int idx, bool clear)
{
	m_renderInfo.getColourAttachment(idx).loadOp = (clear) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
}

void GenericRenderTarget::toggleDepthStencilClear(bool clear)
{
	m_renderInfo.getDepthAttachment().loadOp = (clear) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
}

const RenderInfo &GenericRenderTarget::getRenderInfo() const
{
	return m_renderInfo;
}

uint32_t GenericRenderTarget::getWidth() const
{
	return m_width;
}

uint32_t GenericRenderTarget::getHeight() const
{
	return m_height;
}

RenderTargetType GenericRenderTarget::getType() const
{
	return m_type;
}
