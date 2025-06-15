#include "image.h"

#include "core/common.h"

#include "math/calc.h"

#include "graphics_core.h"

using namespace mgp;

uint32_t clampMipmaps(uint32_t mipmaps, uint32_t w, uint32_t h, uint32_t d)
{
	return CalcU::min(mipmaps, CalcU::floor(CalcU::log2(CalcU::max(w, CalcU::max(h, d)))) + 1);
}

Image::~Image()
{
	if (m_isAllocated)
	{
		vmaDestroyImage(m_gfx->getVMAAllocator(), m_image, m_allocation);

		m_image = VK_NULL_HANDLE;
		m_isAllocated = false;
	}
}

void Image::allocate(
	GraphicsCore *gfx,
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
	m_gfx = gfx;

	m_width = width;
	m_height = height;
	m_depth = depth;

	m_format = format;
	m_type = type;
	m_tiling = tiling;

	m_mipmapCount = clampMipmaps(mipmaps, width, height, depth);
	m_samples = samples;

	if (transient)
	{
		m_usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	else
	{
		m_usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	if (storage)
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

	VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;

	switch (m_type)
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
		MGP_ERROR("Failed to find VkImageType given VkImageViewType: %d", m_type);
		break;
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
		createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	vmaAllocInfo.priority = 1.0f;

	MGP_VK_CHECK(
		vmaCreateImage(m_gfx->getVMAAllocator(), &createInfo, &vmaAllocInfo, &m_image, &m_allocation, &m_allocationInfo),
		"Failed to create image"
	);

	m_isAllocated = true;
}

void Image::wrapAround(
	GraphicsCore *gfx,
	VkImage image,
	VkImageLayout layout,
	unsigned width, unsigned height, unsigned depth,
	VkFormat format,
	VkImageViewType type,
	VkImageTiling tiling,
	uint32_t mipmaps,
	VkSampleCountFlagBits samples,
	VkImageUsageFlags usage
)
{
	m_gfx = gfx;

	m_image = image;
	m_layout = layout;

	m_allocation = nullptr;

	m_width = width;
	m_height = height;
	m_depth = depth;

	m_format = format;
	m_type = type;
	m_tiling = tiling;

	m_mipmapCount = clampMipmaps(mipmaps, width, height, depth);
	m_samples = samples;
	m_usage = usage;

	m_isAllocated = false;
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

	barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

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
	return m_usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
}

bool Image::isStorage() const
{
	return m_usage & VK_IMAGE_USAGE_STORAGE_BIT;
}

bool Image::isCubemap() const
{
	return m_type == VK_IMAGE_VIEW_TYPE_CUBE;
}

bool Image::isDepth() const
{
	return m_format == m_gfx->getDepthFormat();
}

unsigned Image::getLayerCount() const
{
	return (m_type == VK_IMAGE_VIEW_TYPE_1D_ARRAY || m_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY) ? m_depth : getFaceCount();
}

unsigned Image::getFaceCount() const
{
	return isCubemap() ? 6 : 1;
}
