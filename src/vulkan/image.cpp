#include "image.h"

#include "core/common.h"

#include "core.h"
#include "toolbox.h"
#include "image_view.h"

using namespace mgp;

ImageAlloc::ImageAlloc(Image *image)
	: m_parent(image)
	, m_allocation()
	, m_allocationInfo()
{
	VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;

	switch (image->getType())
	{
	case VK_IMAGE_VIEW_TYPE_1D:
	case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		imageType = VK_IMAGE_TYPE_1D;
		break;

	case VK_IMAGE_VIEW_TYPE_2D:
	case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
	case VK_IMAGE_VIEW_TYPE_CUBE:
	case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
		imageType = VK_IMAGE_TYPE_2D;
		break;

	case VK_IMAGE_VIEW_TYPE_3D:
		imageType = VK_IMAGE_TYPE_3D;
		break;

	default:
		MGP_ERROR("Failed to find VkImageType given VkImageViewType: %d", m_parent->getType());
	}

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = imageType;
	createInfo.extent.width = m_parent->getWidth();
	createInfo.extent.height = m_parent->getHeight();
	createInfo.extent.depth = m_parent->getDepth();
	createInfo.mipLevels = m_parent->getMipmapCount();
	createInfo.arrayLayers = m_parent->getFaceCount();
	createInfo.format = m_parent->getFormat();
	createInfo.tiling = m_parent->getTiling();
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = m_parent->getUsage();
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = m_parent->getSamples();
	createInfo.flags = 0;

	if (m_parent->isCubemap())
	{
		createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	MGP_VK_CHECK(
		vmaCreateImage(m_parent->m_core->getVMAAllocator(), &createInfo, &vmaAllocInfo, &m_parent->m_image, &m_allocation, &m_allocationInfo),
		"Failed to create image"
	);

//	getStandardView(); // cache the standard view
};

ImageAlloc::~ImageAlloc()
{
	vmaDestroyImage(m_parent->m_core->getVMAAllocator(), m_parent->m_image, m_allocation);
	m_parent->m_image = VK_NULL_HANDLE;
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
	: m_image(VK_NULL_HANDLE)
	, m_layout()
	, m_allocation(nullptr)
{
	init(
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
}

Image::~Image()
{
	for (auto &[id, view] : m_viewCache)
		delete view;

	delete m_allocation;
}

void Image::init(
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

	m_width = width;
	m_height = height;
	m_depth = depth;

	m_format = format;
	m_type = type;
	m_tiling = tiling;

	m_mipmapCount = mipmaps;
	m_samples = samples;

	m_isTransient = transient;
	m_isStorage = storage;

	if (isTransient())
	{
		m_usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	else
	{
		m_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (isStorage())
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
}

void Image::allocate()
{
	m_allocation = new ImageAlloc(this);
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
		m_core, this,
		layerCount, layer, baseMipLevel
	);

	m_viewCache.insert({ hash, imageView });

	return imageView;
}

ImageView *Image::getStandardView()
{
	return createView(getLayerCount(), 0, 0);
}

VkImageMemoryBarrier2 Image::getBarrier(VkImageLayout newLayout) const
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

VkImageUsageFlags Image::getUsage() const
{
	return m_usage;
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
	return m_isTransient;
}

bool Image::isStorage() const
{
	return m_isStorage;
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
