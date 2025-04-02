#include "image_view.h"

#include "core.h"

using namespace mgp;

ImageView::ImageView(VulkanCore *core, VkImageView view, VkFormat format, VkImageUsageFlags usage)
	: m_view(view)
	, m_format(format)
	, m_bindlessHandle(bindless::INVALID_HANDLE)
	, m_core(core)
{
	if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
		m_bindlessHandle = m_core->getBindlessResources().registerTexture2D(*this);
}

ImageView::~ImageView()
{
	vkDestroyImageView(m_core->getLogicalDevice(), m_view, nullptr);
	m_view = VK_NULL_HANDLE;
}

const VkImageView &ImageView::getHandle() const
{
	return m_view;
}

const VkFormat &ImageView::getFormat() const
{
	return m_format;
}

bindless::Handle ImageView::getBindlessHandle() const
{
	return m_bindlessHandle;
}
