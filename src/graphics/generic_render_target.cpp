#include "generic_render_target.h"

using namespace llt;

GenericRenderTarget::GenericRenderTarget()
	: m_type(RENDER_TARGET_TYPE_NONE)
	, m_width(0)
	, m_height(0)
{
}

GenericRenderTarget::GenericRenderTarget(uint32_t width, uint32_t height)
	: m_type(RENDER_TARGET_TYPE_NONE)
	, m_width(width)
	, m_height(height)
{
}

GenericRenderTarget::~GenericRenderTarget()
{
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
