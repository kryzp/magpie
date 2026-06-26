
static R_TextureInfo R_TextureInfoInitAbsolute(VkFormat format, v3 size)
{
	R_TextureInfo info = {0};
	info.format = format;
	info.size_class = R_SizeClass_Absolute;
	info.relative_to = R_GraphTexHandleNull();
	info.size_x = size.x;
	info.size_y = size.y;
	info.size_z = size.z;
	info.mips = 1;
	info.layers = 1;
	info.samples = 1;
	info.flags = 0;
	
	return info;
}

static R_TextureInfo R_TextureInfoInitSwapchain(VkFormat format, v3 factor)
{
	R_TextureInfo info = {0};
	info.format = format;
	info.size_class = R_SizeClass_SwapchainRelative;
	info.relative_to = R_GraphTexHandleNull();
	info.size_x = factor.x;
	info.size_y = factor.y;
	info.size_z = factor.z;
	info.mips = 1;
	info.layers = 1;
	info.samples = 1;
	info.flags = 0;
	
	return info;
}

static R_TextureInfo R_TextureInfoInitRelative(VkFormat format, v3 factor, R_GraphTexHandle relative_to)
{
	R_TextureInfo info = {0};
	info.format = format;
	info.size_class = R_SizeClass_SwapchainRelative;
	info.relative_to = relative_to;
	info.size_x = factor.x;
	info.size_y = factor.y;
	info.size_z = factor.z;
	info.mips = 1;
	info.layers = 1;
	info.samples = 1;
	info.flags = 0;
	
	return info;
}

static R_BufferInfo R_BufferInfoInit(u64 size,
				 VmaAllocationCreateFlags flags,
				 VkBufferUsageFlags2 usage)
{
	R_BufferInfo info = {0};
	info.size = size;
	info.flags = flags;
	info.usage = usage;

	return info;
}

static b32 R_TextureInfoMatch(const R_TextureInfo *a, const R_TextureInfo *b)
{
	return (a->format     == b->format &&
			a->size_class == b->size_class &&
			R_GraphTexHandleMatch(a->relative_to, b->relative_to) &&
			a->size_x     == b->size_x &&
			a->size_y     == b->size_y &&
			a->size_z     == b->size_z &&
			a->mips       == b->mips &&
			a->layers     == b->layers &&
			a->samples    == b->samples &&
			a->flags      == b->flags);
}

static b32 R_BufferInfoMatch(const R_BufferInfo *a, const R_BufferInfo *b)
{
	return (a->size  == b->size &&
			a->flags == b->flags &&
			a->usage == b->usage);
}
