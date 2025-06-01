#include "image_view.h"

#include "core/common.h"

#include "core.h"
#include "image.h"

using namespace mgp;

ImageView::ImageView(
	VulkanCore *core,
	Image *parent,
	int layerCount,
	int layer,
	int baseMipLevel
)
	: m_view(VK_NULL_HANDLE)
	, m_parent(parent)
	, m_bindlessHandle(BindlessResources::INVALID_HANDLE)
	, m_core(core)
{
	VkImageViewType viewType = m_parent->getType();

	if (m_parent->isCubemap() && layerCount == 1)
	{
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = m_parent->getHandle();
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = m_parent->getFormat();

	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewCreateInfo.subresourceRange.levelCount = m_parent->getMipmapCount() - baseMipLevel;
	viewCreateInfo.subresourceRange.baseArrayLayer = layer;
	viewCreateInfo.subresourceRange.layerCount = layerCount;

	if (m_parent->isDepth())
	{
		// depth AND stencil is not allowed for sampling!
		// so, use depth instead.
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	MGP_VK_CHECK(
		vkCreateImageView(m_core->getLogicalDevice(), &viewCreateInfo, nullptr, &m_view),
		"Failed to create texture image view."
	);

	if (m_parent->getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT)
	{
		if (m_parent->getType() == VK_IMAGE_VIEW_TYPE_2D)
		{
			m_bindlessHandle = m_core->getBindlessResources().registerTexture2D(*this);
		}
		else if (m_parent->getType() == VK_IMAGE_VIEW_TYPE_CUBE)
		{
			m_bindlessHandle = m_core->getBindlessResources().registerCubemap(*this);
		}
	}
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

Image *ImageView::getImage()
{
	return m_parent;
}

const Image *ImageView::getImage() const
{
	return m_parent;
}

uint32_t ImageView::getBindlessHandle() const
{
	return m_bindlessHandle;
}
