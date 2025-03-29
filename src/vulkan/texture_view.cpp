#include "texture_view.h"

#include "vulkan/core.h"

using namespace llt;

TextureView::TextureView(VkImageView view, VkFormat format)
	: m_view(view)
	, m_format(format)
	, m_bindlessHandle(BindlessResourceHandle::INVALID)
{
}

void TextureView::cleanUp()
{
	if (m_view == VK_NULL_HANDLE)
		return;

	vkDestroyImageView(g_vkCore->m_device, m_view, nullptr);
	m_view = VK_NULL_HANDLE;
}

const VkImageView &TextureView::getHandle() const
{
	return m_view;
}

const VkFormat &TextureView::getFormat() const
{
	return m_format;
}

const BindlessResourceHandle &TextureView::getBindlessHandle() const
{
	return m_bindlessHandle;
}
