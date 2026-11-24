#include "graphics/texture.h"
#include "core/log.h"

#include <cstring>

VkImageViewType gfx::Texture::DefaultViewType() const
{
	if (flags & TextureFlag_Cubemap) {
		if (layer_count > 6)
			return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		else
			return VK_IMAGE_VIEW_TYPE_CUBE;
	}

	switch (type) {
		case VK_IMAGE_TYPE_1D:
			if (layer_count > 1)
				return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			else
				return VK_IMAGE_VIEW_TYPE_1D;

		case VK_IMAGE_TYPE_2D:
			if (layer_count > 1)
				return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			else
				return VK_IMAGE_VIEW_TYPE_2D;

		case VK_IMAGE_TYPE_3D:
			// No such thing as VK_IMAGE_VIEW_TYPE_3D_ARRAY.
			return VK_IMAGE_VIEW_TYPE_3D;

		default:
			break;
	}

	AssertTrue(false && "Failed to find image view type for given texture.");

	return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

gfx::SubresourceRange gfx::SubresourceRange::OfTexture(const Texture *texture) const
{
	SubresourceRange result = *this;

	if (result.mips == REMAINING)
		result.mips = texture->mipmap_count - base_mip;

	if (result.layers == REMAINING)
		result.layers = texture->layer_count - base_layer;

	return result;
}

gfx::SubresourceRange gfx::SubresourceRange::All(VkImageAspectFlags aspects)
{
	SubresourceRange range = {};
	range.aspects = aspects;
	range.base_mip = 0;
	range.mips = REMAINING;
	range.base_layer = 0;
	range.layers = REMAINING;

	return range;
}

gfx::SubresourceRange gfx::SubresourceRange::AllColour()
{
	return All(VK_IMAGE_ASPECT_COLOR_BIT);
}

gfx::SubresourceRange gfx::SubresourceRange::AllDepth()
{
	return All(VK_IMAGE_ASPECT_DEPTH_BIT);
}

gfx::TextureViewCreateInfo::TextureViewCreateInfo()
{
	memset((void *)this, 0, sizeof(*this));
}
