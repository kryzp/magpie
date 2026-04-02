
internal VkImageViewType
GFX_TextureDefaultViewType(const GFX_Texture *texture)
{
	if (texture->flags & GFX_TextureFlag_Cubemap)
	{
		if (texture->layer_count > 6)
			return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

		return VK_IMAGE_VIEW_TYPE_CUBE;
	}

	switch (texture->type)
	{
		case VK_IMAGE_TYPE_1D:
			return texture->layer_count > 1
				? VK_IMAGE_VIEW_TYPE_1D_ARRAY
				: VK_IMAGE_VIEW_TYPE_1D;
			
		case VK_IMAGE_TYPE_2D:
			return texture->layer_count > 1
				? VK_IMAGE_VIEW_TYPE_2D_ARRAY
				: VK_IMAGE_VIEW_TYPE_2D;

		case VK_IMAGE_TYPE_3D:
			// No such thing as VK_IMAGE_VIEW_TYPE_3D_ARRAY.
			return VK_IMAGE_VIEW_TYPE_3D;
	}

	AssertTrue(false && "Failed to find image view type for given texture.");

	return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
}

internal GFX_SubresourceRange
GFX_SubresourceRangeOfTexture(const GFX_SubresourceRange *range, const GFX_Texture *texture)
{
	GFX_SubresourceRange result = *range;

	if (result.mips == GFX_SRR_REMAINING_COUNT)
		result.mips = texture->mipmap_count - range->base_mip;

	if (result.layers == GFX_SRR_REMAINING_COUNT)
		result.layers = texture->layer_count - range->base_layer;

	return result;
}

internal GFX_SubresourceRange
GFX_SubresourceRangeAll(VkImageAspectFlags aspects)
{
	GFX_SubresourceRange range = {0};
	range.aspects = aspects;
	range.base_mip = 0;
	range.mips = GFX_SRR_REMAINING_COUNT;
	range.base_layer = 0;
	range.layers = GFX_SRR_REMAINING_COUNT;

	return range;
}

internal GFX_SubresourceRange
GFX_SubresourceRangeAllColour(void)
{
	return GFX_SubresourceRangeAll(VK_IMAGE_ASPECT_COLOR_BIT);
}

internal GFX_SubresourceRange
GFX_SubresourceRangeAllDepth(void)
{
	return GFX_SubresourceRangeAll(VK_IMAGE_ASPECT_DEPTH_BIT);
}
