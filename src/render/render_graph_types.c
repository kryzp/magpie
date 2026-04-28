
internal R_TextureInfo
R_TextureInfoInit(void)
{
	R_TextureInfo info = {0};
	
	info.format = VK_FORMAT_UNDEFINED;

	info.size_class = R_SizeClass_SwapchainRelative;

	info.size_x = 1.f;
	info.size_y = 1.f;
	info.size_z = 1.f;

	info.mips = 1;
	info.layers = 1;
	
	info.samples = 1;

	info.flags = 0;
	
	return info;
}

internal R_TextureInfo
R_TextureInfoInitDepth(GFX_Device *device)
{
	R_TextureInfo info = R_TextureInfoInit();

	info.format = device->context.depth_format;

	return info;
}

internal R_BufferInfo
R_BufferInfoInit(void)
{
	R_BufferInfo info = {0};

	return info;
}

internal b32
R_TextureInfoMatch(const R_TextureInfo *a, const R_TextureInfo *b)
{
	return (a->format     == b->format &&
			a->size_class == b->size_class &&
			a->size_x     == b->size_x &&
			a->size_y     == b->size_y &&
			a->size_z     == b->size_z &&
			a->mips       == b->mips &&
			a->layers     == b->layers &&
			a->samples    == b->samples &&
			a->flags      == b->flags);
}

internal b32
R_BufferInfoMatch(const R_BufferInfo *a, const R_BufferInfo *b)
{
	return (a->size  == b->size &&
			a->flags == b->flags &&
			a->usage == b->usage);
}
