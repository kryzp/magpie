
internal VkImageViewType
G_TextureDefaultViewType(const G_Texture *texture)
{
	if (texture->flags & G_TextureFlag_Cubemap)
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

internal G_SubresourceRange
G_SubresourceRangeOfTexture(const G_SubresourceRange *range, const G_Texture *texture)
{
	G_SubresourceRange result = *range;

	if (result.mips == G_SRR_REMAINING_COUNT)
		result.mips = texture->mipmap_count - range->base_mip;

	if (result.layers == G_SRR_REMAINING_COUNT)
		result.layers = texture->layer_count - range->base_layer;

	return result;
}

internal G_SubresourceRange
G_SubresourceRangeAll(VkImageAspectFlags aspects)
{
	G_SubresourceRange range = {0};
	range.aspects = aspects;
	range.base_mip = 0;
	range.mips = G_SRR_REMAINING_COUNT;
	range.base_layer = 0;
	range.layers = G_SRR_REMAINING_COUNT;

	return range;
}

internal G_SubresourceRange
G_SubresourceRangeAllColour(void)
{
	return G_SubresourceRangeAll(VK_IMAGE_ASPECT_COLOR_BIT);
}

internal G_SubresourceRange
G_SubresourceRangeAllDepth(void)
{
	return G_SubresourceRangeAll(VK_IMAGE_ASPECT_DEPTH_BIT);
}
