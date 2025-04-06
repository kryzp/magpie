#include "image_view.h"

#include "core/common.h"

#include "core.h"
#include "image.h"

using namespace mgp;

ImageView::ImageView(
	VulkanCore *core,
	const ImageInfo &info,
	int layerCount,
	int layer,
	int baseMipLevel
)
	: m_view(VK_NULL_HANDLE)
	, m_imageInfo(info)
	, m_bindlessHandle(bindless::INVALID_HANDLE)
	, m_core(core)
{
	VkImageViewType viewType = info.getType();

	if (info.isCubemap() && layerCount == 1)
	{
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = info.getHandle();
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = info.getFormat();

	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewCreateInfo.subresourceRange.levelCount = info.getMipmapCount() - baseMipLevel;
	viewCreateInfo.subresourceRange.baseArrayLayer = layer;
	viewCreateInfo.subresourceRange.layerCount = layerCount;

	if (info.isDepth())
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

	if (info.getUsage() & VK_IMAGE_USAGE_SAMPLED_BIT)
	{
		if (info.getType() == VK_IMAGE_VIEW_TYPE_2D)
		{
			m_bindlessHandle = m_core->getBindlessResources().registerTexture2D(*this);
		}
		else if (info.getType() == VK_IMAGE_VIEW_TYPE_CUBE)
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

ImageInfo &ImageView::getInfo()
{
	return m_imageInfo;
}

const ImageInfo &ImageView::getInfo() const
{
	return m_imageInfo;
}

bindless::Handle ImageView::getBindlessHandle() const
{
	return m_bindlessHandle;
}
