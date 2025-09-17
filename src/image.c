
internal b32 ImageIsDepth(Image *image)
{
	return image->format == graphics_device->depth_format;
}

internal b32 ImageIsCubemap(Image *image)
{
	return image->type == VK_IMAGE_VIEW_TYPE_CUBE;
}

internal b32 ImageIsStorage(Image *image)
{
	return (image->usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0;
}

internal u32 ImageFaceCount(Image *image)
{
	return ImageIsCubemap(image) ? 6 : 1;
}

internal u32 ImageLayerCount(Image *image)
{
	if (image->type == VK_IMAGE_VIEW_TYPE_1D_ARRAY ||
	    image->type == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
		return image->depth;

	return ImageFaceCount(image);
}

internal u32 ImageClampMipmapCount(u32 mipmaps, u32 w, u32 h, u32 d)
{
	return MinValue(mipmaps, 1u + (u32)(Log2F((f32)MaxValue(w, MaxValue(h, d)))));
}

internal ImageAccessType ImageGetAccessType(Image *image,
					    u32 mip_level,
					    u32 layer,
					    u32 aspect)
{
	return image->access_types[((aspect * ImageLayerCount(image)) + layer) * image->mipmap_count + mip_level];
}

internal void ImageSetAccessType(Image *image,
				 u32 mip_level,
				 u32 layer,
				 u32 aspect,
				 ImageAccessType type)
{
	image->access_types[((aspect * ImageLayerCount(image)) + layer) * image->mipmap_count + mip_level] = type;
}

internal VkImageMemoryBarrier2 ImageGetMemoryBarrier(Image *image,
						     ImageAccessInfo src_access_info,
						     ImageAccessInfo dst_access_info,
						     u32 base_mip_level,
						     u32 level_count,
						     u32 base_array_layer,
						     u32 layer_count)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = image->handle;

	barrier.oldLayout = src_access_info.layout;
	barrier.newLayout = dst_access_info.layout;

	barrier.srcAccessMask = src_access_info.access;
	barrier.dstAccessMask = dst_access_info.access;

	barrier.srcStageMask = src_access_info.stage;
	barrier.dstStageMask = dst_access_info.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.subresourceRange.baseMipLevel = base_mip_level;
	barrier.subresourceRange.levelCount = level_count;
	barrier.subresourceRange.baseArrayLayer = base_array_layer;
	barrier.subresourceRange.layerCount = layer_count;
	barrier.subresourceRange.aspectMask = image->aspect_flags;

	return barrier;
}

internal Image ImageAlloc(u32 width, u32 height, u32 depth,
			  VkFormat format,
			  VkImageViewType type,
			  VkImageTiling tiling,
			  u32 mipmaps,
			  VkSampleCountFlagBits samples,
			  b32 is_transient, b32 is_storage)
{
	Image image = {0};

	image.width = width;
	image.height = height;
	image.depth = depth;

	image.format = format;
	image.type = type;
	image.tiling = tiling;

	image.is_swapchain = false;

	image.mipmap_count = ImageClampMipmapCount(mipmaps, width, height, depth);
	image.samples = samples;

	if (is_transient)
		image.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	else
		image.usage =
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (is_storage)
		image.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (ImageIsDepth(&image))
		image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	else
		image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
	VkImageType image_type = VK_IMAGE_TYPE_MAX_ENUM;

	switch (image.type) {
	case VK_IMAGE_VIEW_TYPE_1D:
	case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		image_type = VK_IMAGE_TYPE_1D;
		break;
		
	case VK_IMAGE_VIEW_TYPE_2D:
	case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
	case VK_IMAGE_VIEW_TYPE_CUBE:
	case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
		image_type = VK_IMAGE_TYPE_2D;
		break;
		
	case VK_IMAGE_VIEW_TYPE_3D:
		image_type = VK_IMAGE_TYPE_3D;
		break;

	default:
		DebugLogCrash("Failed to find VkImageType given VkImageViewType: %d", image.type);
		break;
	}

	image.aspect_flags = ImageIsDepth(&image)
		? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
		: VK_IMAGE_ASPECT_COLOR_BIT;

	image.aspect_count = 0;
	
	for (VkImageAspectFlags b = 1; b <= image.aspect_flags; b <<= 1) {
		if (image.aspect_flags & b)
			image.aspect_count++;
	}

	image.access_count = image.aspect_count * image.mipmap_count * ImageLayerCount(&image);
	image.access_types = MemoryArenaPushC(graphics_device->arena,
					      image.access_count,
					      sizeof(ImageAccessType));
		
	VkImageCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = image_type;
	create_info.extent.width = image.width;
	create_info.extent.height = image.height;
	create_info.extent.depth = image.depth;
	create_info.mipLevels = image.mipmap_count;
	create_info.arrayLayers = ImageLayerCount(&image);
	create_info.format = image.format;
	create_info.tiling = image.tiling;
	create_info.usage = image.usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = image.samples;
	create_info.flags = 0;

	if (ImageIsCubemap(&image))
		create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	vma_alloc_info.priority = 1.f;

	VK_CHECK(vmaCreateImage(graphics_device->vma_allocator, &create_info,
				&vma_alloc_info, &image.handle,
				&image.allocation, &image.allocation_info),
		 "Failed to create image.");

	VkImageMemoryBarrier2 general_layout_barrier = ImageGetMemoryBarrier(&image,
									     SyncGetSrcImageAccessInfo(ImageAccessType_Undefined),
									     SyncGetDstImageAccessInfo(ImageAccessType_General),
									     0, image.mipmap_count,
									     0, ImageLayerCount(&image));

	CommandBuffer cmd = GraphicsBeginInstantSubmit();
	CmdPipelineBarrier(&cmd, 0, 0, NULL, 0, NULL, 1, &general_layout_barrier);
	GraphicsEndInstantSubmit(&cmd);
	
	for (u32 i = 0; i < image.access_count; i++)
		image.access_types[i] = ImageAccessType_General;
	
	return image;
}

internal Image ImageAlloc2D(u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	return ImageAlloc(width, height, 1,
			  format,
			  VK_IMAGE_VIEW_TYPE_2D,
			  VK_IMAGE_TILING_OPTIMAL,
			  mipmaps,
			  VK_SAMPLE_COUNT_1_BIT,
			  false, false);
}

internal Image ImageAlloc2D_RW(u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	return ImageAlloc(width, height, 1,
			  format,
			  VK_IMAGE_VIEW_TYPE_2D,
			  VK_IMAGE_TILING_OPTIMAL,
			  mipmaps,
			  VK_SAMPLE_COUNT_1_BIT,
			  false, true);
}

internal Image ImageAllocDepth2D(u32 width, u32 height, u32 mipmaps)
{
	return ImageAlloc2D(width, height, graphics_device->depth_format, mipmaps);
}

internal Image ImageAllocDepth2D_RW(u32 width, u32 height, u32 mipmaps)
{
	return ImageAlloc2D_RW(width, height, graphics_device->depth_format, mipmaps);
}

internal Image ImageAllocCubemap(u32 resolution, VkFormat format, u32 mipmaps)
{
	return ImageAlloc(resolution, resolution, 1,
			  format,
			  VK_IMAGE_VIEW_TYPE_CUBE,
			  VK_IMAGE_TILING_OPTIMAL,
			  mipmaps,
			  VK_SAMPLE_COUNT_1_BIT,
			  false, false);
}

internal void ImageDestroy(Image *image)
{
	vmaDestroyImage(graphics_device->vma_allocator, image->handle, image->allocation);
	image->handle = VK_NULL_HANDLE;
}

internal void ImageViewDestroy(ImageView *view)
{
	vkDestroyImageView(graphics_device->device, view->handle, NULL);
	view->handle = VK_NULL_HANDLE;
}

internal ImageView ImageViewFromImage(Image *image,
				      u32 layer_count,
				      u32 layer,
				      u32 base_mip_level,
				      VkImageAspectFlags aspect)
{
	VkImageViewType view_type = image->type;

	if (ImageIsCubemap(image) && layer_count == 1)
		view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = image->handle;
	view_create_info.viewType = view_type;
	view_create_info.format = image->format;

	view_create_info.subresourceRange.aspectMask = aspect;
	view_create_info.subresourceRange.baseMipLevel = base_mip_level;
	view_create_info.subresourceRange.levelCount = image->mipmap_count - base_mip_level;
	view_create_info.subresourceRange.baseArrayLayer = layer;
	view_create_info.subresourceRange.layerCount = layer_count;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	ImageView view = {0};
	view.image = image;
	view.layer_count = layer_count;
	view.layer = layer;
	view.mip_level_count = image->mipmap_count - base_mip_level;
	view.base_mip_level = base_mip_level;
	view.aspect = aspect;
	
	VK_CHECK(vkCreateImageView(graphics_device->device,
				   &view_create_info, NULL,
				   &view.handle),
		 "Failed to create texture image view.");

	// Swapchain images are omitted from being accessible bindlessly.
	if (!image->is_swapchain)
		view.bindless = BindlessRegisterImage(&graphics_device->bindless, view.handle,
						      true, ImageIsStorage(image));
	
	return view;
}

internal ImageView *FetchImageView(Image *image,
				   u32 layer_count,
				   u32 layer,
				   u32 base_mip_level)
{
	u64 hash = 0;

	hash = HashBytesGenericCombine(hash,  image,          sizeof(Image)); // TODO: Hash with a HashImage() function that doesn't just hash the whole image incl. vulkan handle?
	hash = HashBytesGenericCombine(hash, &layer_count,    sizeof(u32));
	hash = HashBytesGenericCombine(hash, &layer,          sizeof(u32));
	hash = HashBytesGenericCombine(hash, &base_mip_level, sizeof(u32));

	ImageView *fetched_image_view = HashTableFetchElement(&graphics_device->image_view_cache, hash);

	if (fetched_image_view)
		return fetched_image_view;

	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

	if (ImageIsDepth(image))
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	
	ImageView view = ImageViewFromImage(image, layer_count, layer, base_mip_level, aspect);

	return HashTableAddElement(&graphics_device->image_view_cache, hash, &view);
}

internal ImageView *FetchStandardImageView(Image *image)
{
	return FetchImageView(image, ImageLayerCount(image), 0, 0);
}

internal BindlessImageHandle FetchStandardImageViewID(Image *image)
{
	return FetchStandardImageView(image)->bindless;
}
