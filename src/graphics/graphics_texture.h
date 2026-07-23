#ifndef GRAPHICS_TEXTURE_H
#define GRAPHICS_TEXTURE_H

typedef u32 G_TextureFlags;
enum
{
	G_TextureFlag_None      = 0,
	G_TextureFlag_Transient = 1 << 0,
	G_TextureFlag_Depth     = 1 << 1,
	G_TextureFlag_Storage   = 1 << 2,
	G_TextureFlag_Cubemap   = 1 << 3,
	G_TextureFlag_Swapchain = 1 << 4
};

typedef struct G_Texture G_Texture;
struct G_Texture
{
	VkImage vk_handle;
	VkImageUsageFlags usage;

	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;

	u32 width;
	u32 height;
	u32 depth;

	G_TextureFlags flags;
	
	VkFormat format;
	VkImageType type;
	VkImageTiling tiling;

	VkImageAspectFlags aspect_flags;

	u32 layer_count;
	u32 mipmap_count;

	VkSampleCountFlagBits sample_count;
};

internal VkImageViewType G_TextureDefaultViewType(const G_Texture *texture);

#define G_SRR_REMAINING_COUNT ((u32)(-1))

typedef struct G_SubresourceRange G_SubresourceRange;
struct G_SubresourceRange
{
	VkImageAspectFlags aspects;
	u32 base_mip;
	u32 mips;
	u32 base_layer;
	u32 layers;
};

internal G_SubresourceRange G_SubresourceRangeOfTexture(const G_SubresourceRange *range, const G_Texture *texture);
internal G_SubresourceRange G_SubresourceRangeAll(VkImageAspectFlags aspects);
internal G_SubresourceRange G_SubresourceRangeAllColour(void);
internal G_SubresourceRange G_SubresourceRangeAllDepth(void);

typedef struct G_TextureView G_TextureView;
struct G_TextureView
{
	VkImageView vk_handle;
	VkImageViewType type;
	G_SubresourceRange range;
	u32 bindless;
};

#endif // GRAPHICS_TEXTURE_H
