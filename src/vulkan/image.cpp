#include "image.h"

#include "core/common.h"

#include "core.h"
#include "toolbox.h"
#include "image_view.h"

using namespace mgp;

Image::Image()
	: m_image(VK_NULL_HANDLE)
	, m_layout()
	, m_viewCache()
	, m_core(nullptr)
	, m_width(0)
	, m_height(0)
	, m_depth(1)
	, m_format()
	, m_type()
	, m_tiling()
	, m_usage()
	, m_mipmapCount(1)
	, m_samples(VK_SAMPLE_COUNT_1_BIT)
	, m_allocation()
	, m_allocationInfo()
	, m_transient(false)
	, m_isUAV(false)
{
}

Image::Image(
	VulkanCore *core,
	unsigned width, unsigned height, unsigned depth,
	VkFormat format,
	VkImageViewType type,
	VkImageTiling tiling,
	uint32_t mipmaps,
	VkSampleCountFlagBits samples,
	bool transient,
	bool uav
)
	: Image()
{
	create(
		core,
		width, height, depth,
		format,
		type,
		tiling,
		mipmaps,
		samples,
		transient,
		uav
	);
};

Image::~Image()
{
	for (auto &[id, view] : m_viewCache)
		delete view;

	vmaDestroyImage(m_core->getVMAAllocator(), m_image, m_allocation);
	m_image = VK_NULL_HANDLE;
}

void Image::create(
	VulkanCore *core,
	unsigned width, unsigned height, unsigned depth,
	VkFormat format,
	VkImageViewType type,
	VkImageTiling tiling,
	uint32_t mipmaps,
	VkSampleCountFlagBits samples,
	bool transient,
	bool uav
)
{
	m_core = core;

	m_width = width;
	m_height = height;
	m_depth = depth;

	m_format = format;
	m_type = type;
	m_tiling = tiling;

	m_mipmapCount = mipmaps;
	m_samples = samples;

	m_transient = transient;
	m_isUAV = uav;

	VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;

	switch (m_type)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			imageType = VK_IMAGE_TYPE_1D;
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			imageType = VK_IMAGE_TYPE_1D;
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			imageType = VK_IMAGE_TYPE_3D;
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		default:
			MGP_ERROR("Failed to find VkImageType given VkImageViewType: %d", m_type);
	}

	if (m_transient)
	{
		m_usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	else
	{
		m_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if (isUAV())
	{
		m_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if (isDepth())
	{
		m_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else
	{
		m_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = imageType;
	createInfo.extent.width = m_width;
	createInfo.extent.height = m_height;
	createInfo.extent.depth = m_depth;
	createInfo.mipLevels = m_mipmapCount;
	createInfo.arrayLayers = getFaceCount();
	createInfo.format = m_format;
	createInfo.tiling = m_tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = m_usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = m_samples;
	createInfo.flags = 0;

	if (isCubemap())
	{
		createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	else
	{
		createInfo.flags = 0;
	}

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	MGP_VK_CHECK(
		vmaCreateImage(m_core->getVMAAllocator(), &createInfo, &vmaAllocInfo, &m_image, &m_allocation, &m_allocationInfo),
		"Failed to create image"
	);

//	getStandardView(); // cache the standard view
}

ImageView *Image::createView(
	int layerCount,
	int layer,
	int baseMipLevel
)
{
	uint64_t hash = 0;

	hash::combine(&hash, &layerCount);
	hash::combine(&hash, &layer);
	hash::combine(&hash, &baseMipLevel);

	if (m_viewCache.contains(hash))
		return m_viewCache.at(hash);

	VkImageViewType viewType = m_type;

	if (isCubemap() && layerCount == 1)
	{
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = m_image;
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = m_format;

	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewCreateInfo.subresourceRange.levelCount = m_mipmapCount - baseMipLevel;
	viewCreateInfo.subresourceRange.baseArrayLayer = layer;
	viewCreateInfo.subresourceRange.layerCount = layerCount;

	if (isDepth())
	{
		// depth AND stencil is not allowed for sampling!
		// so, use depth instead.
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageView view = {};

	MGP_VK_CHECK(
		vkCreateImageView(m_core->getLogicalDevice(), &viewCreateInfo, nullptr, &view),
		"Failed to create texture image view."
	);

	ImageView *imageView = new ImageView(m_core, view, m_type, m_format, m_usage);

	m_viewCache.insert({ hash, imageView });

	return imageView;
}

VkImageMemoryBarrier2 Image::getBarrier(
	VkImageLayout newLayout
) const
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.oldLayout = m_layout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = m_image;

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_mipmapCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = getLayerCount();

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	barrier.srcStageMask = 0;
	barrier.dstStageMask = 0;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	return barrier;
}

const VkImage &Image::getHandle() const
{
	return m_image;
}

const VkImageLayout &Image::getLayout() const
{
	return m_layout;
}

unsigned Image::getWidth() const
{
	return m_width;
}

unsigned Image::getHeight() const
{
	return m_height;
}

unsigned Image::getDepth() const
{
	return m_depth;
}

VkFormat Image::getFormat() const
{
	return m_format;
}

VkImageViewType Image::getType() const
{
	return m_type;
}

VkImageTiling Image::getTiling() const
{
	return m_tiling;
}

uint32_t Image::getMipmapCount() const
{
	return m_mipmapCount;
}

VkSampleCountFlagBits Image::getSamples() const
{
	return m_samples;
}

bool Image::isTransient() const
{
	return m_transient;
}

bool Image::isUAV() const
{
	return m_isUAV;
}

bool Image::isCubemap() const
{
	return m_type == VK_IMAGE_VIEW_TYPE_CUBE;
}

bool Image::isDepth() const
{
	return vk_toolbox::hasStencilComponent(m_format);
}

unsigned Image::getLayerCount() const
{
	return (m_type == VK_IMAGE_VIEW_TYPE_1D_ARRAY || m_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY) ? m_depth : getFaceCount();
}

unsigned Image::getFaceCount() const
{
	return isCubemap() ? 6 : 1;
}

const ImageView *Image::getStandardView()
{
	return createView(getLayerCount(), 0, 0);
}
