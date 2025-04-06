#include "image.h"

#include "core/common.h"

#include "core.h"
#include "toolbox.h"
#include "image_view.h"

using namespace mgp;

ImageInfo::ImageInfo()
	: m_image(VK_NULL_HANDLE)
	, m_layout()
	, m_width(0)
	, m_height(0)
	, m_depth(1)
	, m_format()
	, m_type()
	, m_tiling()
	, m_usage()
	, m_mipmapCount(1)
	, m_samples(VK_SAMPLE_COUNT_1_BIT)
	, m_transient(false)
	, m_storage(false)
{
}

ImageInfo::~ImageInfo()
{
}

VkImageMemoryBarrier2 ImageInfo::getBarrier(VkImageLayout newLayout) const
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
	barrier.subresourceRange.levelCount = getMipmapCount();
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

const VkImage &ImageInfo::getHandle() const
{
	return m_image;
}

const VkImageLayout &ImageInfo::getLayout() const
{
	return m_layout;
}

unsigned ImageInfo::getWidth() const
{
	return m_width;
}

unsigned ImageInfo::getHeight() const
{
	return m_height;
}

unsigned ImageInfo::getDepth() const
{
	return m_depth;
}

VkFormat ImageInfo::getFormat() const
{
	return m_format;
}

VkImageViewType ImageInfo::getType() const
{
	return m_type;
}

VkImageTiling ImageInfo::getTiling() const
{
	return m_tiling;
}

VkImageUsageFlags ImageInfo::getUsage() const
{
	return m_usage;
}

uint32_t ImageInfo::getMipmapCount() const
{
	return m_mipmapCount;
}

VkSampleCountFlagBits ImageInfo::getSamples() const
{
	return m_samples;
}

bool ImageInfo::isTransient() const
{
	return m_transient;
}

bool ImageInfo::isStorage() const
{
	return m_storage;
}

bool ImageInfo::isCubemap() const
{
	return m_type == VK_IMAGE_VIEW_TYPE_CUBE;
}

bool ImageInfo::isDepth() const
{
	return vk_toolbox::hasStencilComponent(m_format);
}

unsigned ImageInfo::getLayerCount() const
{
	return (m_type == VK_IMAGE_VIEW_TYPE_1D_ARRAY || m_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY) ? m_depth : getFaceCount();
}

unsigned ImageInfo::getFaceCount() const
{
	return isCubemap() ? 6 : 1;
}

Image::Image()
	: m_info()
	, m_viewCache()
	, m_allocation()
	, m_allocationInfo()
	, m_core(nullptr)
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
	bool storage
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
		storage
	);
};

Image::~Image()
{
	for (auto &[id, view] : m_viewCache)
		delete view;

	vmaDestroyImage(m_core->getVMAAllocator(), m_info.m_image, m_allocation);
	m_info.m_image = VK_NULL_HANDLE;
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
	bool storage
)
{
	m_core = core;

	m_info.m_width = width;
	m_info.m_height = height;
	m_info.m_depth = depth;

	m_info.m_format = format;
	m_info.m_type = type;
	m_info.m_tiling = tiling;

	m_info.m_mipmapCount = mipmaps;
	m_info.m_samples = samples;

	m_info.m_transient = transient;
	m_info.m_storage = storage;

	VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;

	switch (type)
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
			MGP_ERROR("Failed to find VkImageType given VkImageViewType: %d", type);
	}

	if (transient)
	{
		m_info.m_usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	else
	{
		m_info.m_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (storage)
	{
		m_info.m_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if (m_info.isDepth())
	{
		m_info.m_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else
	{
		m_info.m_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = imageType;
	createInfo.extent.width = m_info.m_width;
	createInfo.extent.height = m_info.m_height;
	createInfo.extent.depth = m_info.m_depth;
	createInfo.mipLevels = m_info.m_mipmapCount;
	createInfo.arrayLayers = m_info.getFaceCount();
	createInfo.format = m_info.m_format;
	createInfo.tiling = m_info.m_tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = m_info.m_usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = m_info.m_samples;
	createInfo.flags = 0;

	if (m_info.isCubemap())
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
		vmaCreateImage(m_core->getVMAAllocator(), &createInfo, &vmaAllocInfo, &m_info.m_image, &m_allocation, &m_allocationInfo),
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

	ImageView *imageView = new ImageView(
		m_core, m_info,
		layerCount, layer, baseMipLevel
	);

	m_viewCache.insert({ hash, imageView });

	return imageView;
}

ImageView *Image::getStandardView()
{
	return createView(m_info.getLayerCount(), 0, 0);
}

const VkImage &Image::getHandle() const
{
	return m_info.getHandle();
}

ImageInfo &Image::getInfo()
{
	return m_info;
}

const ImageInfo &Image::getInfo() const
{
	return m_info;
}
