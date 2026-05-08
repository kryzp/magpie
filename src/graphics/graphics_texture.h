#ifndef GRAPHICS_TEXTURE_H
#define GRAPHICS_TEXTURE_H

typedef u32 GFX_TextureFlags;
enum
{
	GFX_TextureFlag_None      = 0,
	GFX_TextureFlag_Transient = 1 << 0,
	GFX_TextureFlag_Depth     = 1 << 1,
	GFX_TextureFlag_Storage   = 1 << 2,
	GFX_TextureFlag_Cubemap   = 1 << 3,
	GFX_TextureFlag_Swapchain = 1 << 4
};

typedef struct GFX_Texture GFX_Texture;
struct GFX_Texture
{
	VkImage vk_handle;
	VkImageUsageFlags usage;

	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;

	u32 width;
	u32 height;
	u32 depth;

	GFX_TextureFlags flags;
	
	VkFormat format;
	VkImageType type;
	VkImageTiling tiling;

	VkImageAspectFlags aspect_flags;

	u32 layer_count;
	u32 mipmap_count;

	VkSampleCountFlagBits sample_count;
};

internal VkImageViewType GFX_TextureDefaultViewType(const GFX_Texture *texture);

#define GFX_SRR_REMAINING_COUNT ((u32)(-1))

typedef struct GFX_SubresourceRange GFX_SubresourceRange;
struct GFX_SubresourceRange
{
	VkImageAspectFlags aspects;
	u32 base_mip;
	u32 mips;
	u32 base_layer;
	u32 layers;
};

internal GFX_SubresourceRange GFX_SubresourceRangeOfTexture(const GFX_SubresourceRange *range, const GFX_Texture *texture);
internal GFX_SubresourceRange GFX_SubresourceRangeAll(VkImageAspectFlags aspects);
internal GFX_SubresourceRange GFX_SubresourceRangeAllColour(void);
internal GFX_SubresourceRange GFX_SubresourceRangeAllDepth(void);

typedef struct GFX_TextureView GFX_TextureView;
struct GFX_TextureView
{
	VkImageView vk_handle;
	VkImageViewType type;
	GFX_SubresourceRange range;

	GFX_BindlessHandle bindless;
};

#endif // GRAPHICS_TEXTURE_H
