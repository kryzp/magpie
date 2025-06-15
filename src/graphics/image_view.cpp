#include "image_view.h"

#include "core/common.h"

#include "graphics_core.h"
#include "image.h"

using namespace mgp;

ImageView::ImageView(
	GraphicsCore *gfx,
	Image *image,
	int layerCount,
	int layer,
	int baseMipLevel
)
	: m_gfx(gfx)
	, m_view(VK_NULL_HANDLE)
	, m_image(image)
{
	VkImageViewType viewType = m_image->getType();

	if (m_image->isCubemap() && layerCount == 1)
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = m_image->getHandle();
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = m_image->getFormat();

	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewCreateInfo.subresourceRange.levelCount = m_image->getMipmapCount() - baseMipLevel;
	viewCreateInfo.subresourceRange.baseArrayLayer = layer;
	viewCreateInfo.subresourceRange.layerCount = layerCount;

	if (m_image->isDepth())
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
		vkCreateImageView(m_gfx->getLogicalDevice(), &viewCreateInfo, nullptr, &m_view),
		"Failed to create texture image view."
	);
}

ImageView::~ImageView()
{
	vkDestroyImageView(m_gfx->getLogicalDevice(), m_view, nullptr);
	m_view = VK_NULL_HANDLE;
}

const VkImageView &ImageView::getHandle() const
{
	return m_view;
}

Image *ImageView::getImage()
{
	return m_image;
}

const Image *ImageView::getImage() const
{
	return m_image;
}

void ImageViewCache::init(GraphicsCore *gfx)
{
	m_gfx = gfx;
}

void ImageViewCache::destroy()
{
	for (auto &[id, view] : m_viewCache)
		delete view;

	m_viewCache.clear();
}

ImageView *ImageViewCache::fetchStdView(Image *image)
{
	return fetchView(image, image->getLayerCount(), 0, 0);
}

ImageView *ImageViewCache::fetchView(
	Image *image,
	int layerCount,
	int layer,
	int baseMipLevel
)
{
	uint64_t hash = 0;

	hash::combine(&hash, image); // todo: DONT DO THIS. REPLACE WITH AN image->getHash() function!!!
	hash::combine(&hash, &layerCount);
	hash::combine(&hash, &layer);
	hash::combine(&hash, &baseMipLevel);

	if (m_viewCache.contains(hash))
		return m_viewCache.at(hash);

	ImageView *view = m_gfx->createImageView(image, layerCount, layer, baseMipLevel);

	m_viewCache.insert({ hash, view });

	return view;
}
