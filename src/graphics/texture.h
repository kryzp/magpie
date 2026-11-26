#ifndef GRAPHICS_TEXTURE_H
#define GRAPHICS_TEXTURE_H

#include "core/types.h"
#include "graphics/resource_key.h"

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace gfx {

enum TextureFlags : unsigned {
	TextureFlag_None      = 0,
	TextureFlag_Transient = 1 << 0,
	TextureFlag_Depth     = 1 << 1,
	TextureFlag_Storage   = 1 << 2,
	TextureFlag_Cubemap   = 1 << 3,
	TextureFlag_Swapchain = 1 << 4
};

enum TextureAllocFlags : unsigned {
	TextureAllocFlag_None      = 0,
	TextureAllocFlag_Transient = 1 << 0,
	TextureAllocFlag_Storage   = 1 << 1,
	TextureAllocFlag_Cubemap   = 1 << 2
};

struct TextureAllocInfo {
	unsigned width = 1;
	unsigned height = 1;
	unsigned depth = 1;

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
	VkImageType type = VK_IMAGE_TYPE_2D;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

	unsigned mipmaps = 1;
	unsigned layers = 1;

	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

	unsigned flags = TextureAllocFlag_None;
};

struct Texture {
	VkImage vk_handle = VK_NULL_HANDLE;
	VkImageUsageFlags usage = 0;

	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocation_info = {};

	unsigned width = 0;
	unsigned height = 0;
	unsigned depth = 0;

	unsigned flags = TextureFlag_None;

	VkFormat format = VK_FORMAT_UNDEFINED;
	VkImageType type = VK_IMAGE_TYPE_2D;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

	VkImageAspectFlags aspect_flags = 0;

	unsigned layer_count = 0;
	unsigned mipmap_count = 0;

	VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;

	VkImageViewType DefaultViewType() const;
};

// hashed byte for byte
struct SubresourceRange {

	static constexpr  unsigned REMAINING = static_cast<unsigned>(-1);

	VkImageAspectFlags aspects;
	unsigned base_mip;
	unsigned mips;
	unsigned base_layer;
	unsigned layers;

	SubresourceRange OfTexture(const Texture *texture) const;

	static SubresourceRange All(VkImageAspectFlags aspects);
	static SubresourceRange AllColour();
	static SubresourceRange AllDepth();
};

struct TextureViewCreateInfo {
	ResourceKey texture;
	VkImageViewType type;
	SubresourceRange range;

	TextureViewCreateInfo();
};

struct TextureView {
	VkImageView vk_handle = VK_NULL_HANDLE;
	VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
	SubresourceRange range = {};
	unsigned bindless = 0;
};

inline bool TextureFormatHasStencil(VkFormat format)
{
	return (
		format == VK_FORMAT_S8_UINT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT ||
		format == VK_FORMAT_D32_SFLOAT_S8_UINT
	);
}

inline bool TextureFormatIsDepth(VkFormat format)
{
	return (
		format == VK_FORMAT_D16_UNORM ||
		format == VK_FORMAT_X8_D24_UNORM_PACK32 ||
		format == VK_FORMAT_D32_SFLOAT ||
		format == VK_FORMAT_D16_UNORM_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT ||
		format == VK_FORMAT_D32_SFLOAT_S8_UINT
	);
}

inline bool TextureFormatIsBlockCompressed(VkFormat format)
{
	return (
		(format >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && format <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK) ||
		(format >= VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG && format <= VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG) ||
		(format >= VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK && format <= VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK)
	);
}

}

#endif // GRAPHICS_TEXTURE_H
