
#define VK_CHECK(_func_call, _error_msg) \
do { \
VkResult _vk_check_result = _func_call; \
if(_vk_check_result != VK_SUCCESS) { \
DebugLogCrash(_error_msg); \
} \
} while(0)

internal VkFormat
FindGraphicsSupportedFormat(VkPhysicalDevice physical_device,
							VkImageTiling tiling,
							VkFormatFeatureFlags features,
							u32 candidate_count,
							VkFormat *candidates)
{
	for(i32 i = 0; i < candidate_count; i++)
	{
		VkFormatProperties properties = {0};
		vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &properties);
		
		if((tiling == VK_IMAGE_TILING_LINEAR  && (properties.linearTilingFeatures  & features) == features) ||
		   (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features))
		{
			return candidates[i];
		}
		
	}
	
	DebugLogCrash("Failed to find supported format.");
	
	return VK_FORMAT_MAX_ENUM;
}

internal VkFormat
FindGraphicsDepthFormat(VkPhysicalDevice physical_device)
{
	static VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};
	
	return FindGraphicsSupportedFormat(physical_device,
									   VK_IMAGE_TILING_OPTIMAL,
									   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
									   ArraySize(candidates), candidates);
}

internal VkSampleCountFlagBits
FindGraphicsMaxUsableSampleCount(VkPhysicalDeviceProperties2 properties)
{
	VkSampleCountFlags counts =
		properties.properties.limits.framebufferColorSampleCounts &
		properties.properties.limits.framebufferDepthSampleCounts;
	
	if(counts & VK_SAMPLE_COUNT_64_BIT)
	{
		return VK_SAMPLE_COUNT_64_BIT;
	}
	else if(counts & VK_SAMPLE_COUNT_32_BIT)
	{
		return VK_SAMPLE_COUNT_32_BIT;
	}
	else if(counts & VK_SAMPLE_COUNT_16_BIT)
	{
		return VK_SAMPLE_COUNT_16_BIT;
	}
	else if(counts & VK_SAMPLE_COUNT_8_BIT)
	{
		return VK_SAMPLE_COUNT_8_BIT;
	}
	else if(counts & VK_SAMPLE_COUNT_4_BIT)
	{
		return VK_SAMPLE_COUNT_4_BIT;
	}
	else if(counts & VK_SAMPLE_COUNT_2_BIT)
	{
		return VK_SAMPLE_COUNT_2_BIT;
	}
	else if(counts & VK_SAMPLE_COUNT_1_BIT)
	{
		return VK_SAMPLE_COUNT_1_BIT;
	}
	
	DebugLogCrash("Could not find a maximum usable sample count.");
	
	return VK_SAMPLE_COUNT_1_BIT;
}

internal void
GraphicsWaitIdle()
{
	vkDeviceWaitIdle(graphics_device->device);
}

internal void
WaitForFence(VkFence fence)
{
	vkWaitForFences(graphics_device->device, 1, &fence, VK_TRUE, UINT64_MAX);
}

internal void
ResetFence(VkFence fence)
{
	vkResetFences(graphics_device->device, 1, &fence);
}

internal VkSemaphore
GetCurrentImageAvailableSemaphore()
{
	return graphics_device->frames[graphics_device->current_frame_index].image_available_semaphore;
}

internal VkSemaphore
GetCurrentRenderFinishedSemaphore()
{
	return graphics_device->frames[graphics_device->current_frame_index].render_finished_semaphore;
}

internal SwapchainSupportDetails
QuerySwapchainSupport(MemoryArena *arena, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	SwapchainSupportDetails result = {0};
	
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &result.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &result.surface_format_count, 0);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &result.present_mode_count, 0);
	
	if(result.surface_format_count >= 0)
	{
		result.surface_formats = MemoryArenaPush(arena, sizeof(VkSurfaceFormatKHR) * result.surface_format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &result.surface_format_count, result.surface_formats);
	}
	
	if(result.present_mode_count >= 0)
	{
		result.present_modes = MemoryArenaPush(arena, sizeof(VkPresentModeKHR) * result.present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &result.present_mode_count, result.present_modes);
	}
	
	return result;
}

internal const char *const *
GetInstanceExtensions(MemoryArena *arena, Platform *platform, u32 *extension_count)
{
	const char *const *names = platform->GetVulkanInstanceExtensions(extension_count);
	
	if(!names)
	{
		DebugLogCrash("Unable to get instance extension count.");
	}
	
	u32 extra_extension_count = 3;
	
#ifdef __APPLE__
	extra_extension_count += 2;
#endif
	
	const char **extensions = MemoryArenaPush(arena, sizeof(const char *) * (*extension_count + extra_extension_count));
	
	for(i32 i = 0; i < *extension_count; i++)
	{
		extensions[i] = names[i];
	}
	
	extensions[*extension_count + 0] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
	extensions[*extension_count + 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	extensions[*extension_count + 2] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	
#ifdef __APPLE__
	extensions[*extension_count + 3] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	extensions[*extension_count + 4] = "VK_EXT_metal_surface";
#endif
	
	(*extension_count) = (*extension_count) + extra_extension_count;
	
	return extensions;
}

internal VkSurfaceFormatKHR
GraphicsChooseSwapSurfaceFormat(VkSurfaceFormatKHR *available_surface_formats, u32 available_surface_format_count)
{
	VkSurfaceFormatKHR *format = available_surface_formats;
	
	for(i32 i = 0; i < available_surface_format_count; i++, format++)
	{
		if(format->format == VK_FORMAT_B8G8R8A8_UNORM &&
		   format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			DebugLog("Found *desired* swapchain swap surface format & colour space!");
			
			return *format;
		}
	}
	
	DebugLog("Could not find *desired* swapchain swap surface format & colour space, falling back...");
	
	return *available_surface_formats;
}

internal VkPresentModeKHR
GraphicsChooseSwapPresentMode(VkPresentModeKHR *available_present_modes, u32 available_present_mode_count, b32 enable_vsync)
{
	if(!enable_vsync)
	{
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
	
	VkPresentModeKHR *mode = available_present_modes;
	
	for(i32 i = 0; i < available_present_mode_count; i++, mode++)
	{
		if(*mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return VK_PRESENT_MODE_MAILBOX_KHR;
		}
	}
	
	return VK_PRESENT_MODE_FIFO_KHR;
}

internal VkExtent2D
GraphicsChooseSwapExtent(Platform *platform, const VkSurfaceCapabilitiesKHR *capabilities)
{
	if(capabilities->currentExtent.width != (u32)(-1) &&
	   capabilities->currentExtent.height != (u32)(-1))
	{
		return capabilities->currentExtent;
	}
	
	VkExtent2D actual_extent = { platform->window_width, platform->window_height };
	
	actual_extent.width = ClampValue(actual_extent.width,
									 capabilities->minImageExtent.width,
									 capabilities->maxImageExtent.width);
	
	actual_extent.height = ClampValue(actual_extent.height,
									  capabilities->minImageExtent.height,
									  capabilities->maxImageExtent.height);
	
	return actual_extent;
}

internal b32
BindlessIsValid(u32 resource_id)
{
	return resource_id != 0;
}

internal u32
BindlessRegisterSampledImage(VkImageView view, b32 is_depth)
{
	BindlessUpdate *update = graphics_device->bindless_updates + graphics_device->n_bindless_updates;
	
	update->type = BindlessUpdateType_SampledImage;
	update->slot = ++graphics_device->n_bindless_sampled_images;
	
	update->sampled_image.view = view;
	update->sampled_image.is_depth = is_depth;
	
	graphics_device->n_bindless_updates++;
	
	return update->slot;
}

internal u32
BindlessRegisterSampler(VkSampler sampler)
{
	BindlessUpdate *update = graphics_device->bindless_updates + graphics_device->n_bindless_updates;
	
	update->type = BindlessUpdateType_Sampler;
	update->slot = ++graphics_device->n_bindless_samplers;
	
	update->sampler.sampler = sampler;
	
	graphics_device->n_bindless_updates++;
	
	return update->slot;
}

internal void
BindlessInit()
{
	static VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, BINDLESS_MAX_RESOURCES }, // Textures.
		{ VK_DESCRIPTOR_TYPE_SAMPLER,       BINDLESS_MAX_RESOURCES }  // Samplers.
	};
	
	VkDescriptorPoolCreateInfo pool_create_info = {0};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = BINDLESS_MAX_RESOURCES * ArraySize(pool_sizes);
	pool_create_info.poolSizeCount = ArraySize(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;
	
	VK_CHECK(vkCreateDescriptorPool(graphics_device->device, &pool_create_info, 0, &graphics_device->bindless_pool),
			 "Failed to create bindless descriptor pool.");
	
	// ---
	
	VkDescriptorBindingFlags bindless_flags[2] = {
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT
	};
	
	VkDescriptorSetLayoutBinding bindings[2] = {0};
	{
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		bindings[0].descriptorCount = BINDLESS_MAX_RESOURCES;
		bindings[0].binding = BINDLESS_SAMPLED_IMAGE_BINDING;
		bindings[0].stageFlags = VK_SHADER_STAGE_ALL;
		bindings[0].pImmutableSamplers = 0;
		
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		bindings[1].descriptorCount = BINDLESS_MAX_RESOURCES;
		bindings[1].binding = BINDLESS_SAMPLER_BINDING;
		bindings[1].stageFlags = VK_SHADER_STAGE_ALL;
		bindings[1].pImmutableSamplers = 0;
	}
	
	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {0};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = 2;
	binding_flags.pBindingFlags = bindless_flags;
	
	VkDescriptorSetLayoutCreateInfo layout_create_info = {0};
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = 2;
	layout_create_info.pBindings = bindings;
	layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
	layout_create_info.pNext = &binding_flags;
	
	VK_CHECK(vkCreateDescriptorSetLayout(graphics_device->device, &layout_create_info, 0, &graphics_device->bindless_layout),
			 "Failed to create bindless descriptor layout.");
	
	// ---
	
	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = graphics_device->bindless_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &graphics_device->bindless_layout;
	
	VK_CHECK(vkAllocateDescriptorSets(graphics_device->device, &alloc_info, &graphics_device->bindless_set),
			 "Failed to allocate bindless descriptor set.");
	
	DebugLog("Bindless resources created.");
}

internal void
BindlessDestroy()
{
	vkDestroyDescriptorSetLayout(graphics_device->device, graphics_device->bindless_layout, 0);
	vkDestroyDescriptorPool(graphics_device->device, graphics_device->bindless_pool, 0);
}

internal void
BindlessApplyUpdates()
{
	VkWriteDescriptorSet descriptor_writes[BINDLESS_MAX_WRITES_PER_FRAME] = {0};
	VkDescriptorImageInfo image_infos[BINDLESS_MAX_WRITES_PER_FRAME] = {0};
	
	for(i32 i = 0; i < graphics_device->n_bindless_updates; i++)
	{
		BindlessUpdate *update = graphics_device->bindless_updates + i;
		
		switch(update->type)
		{
			case BindlessUpdateType_SampledImage:
			{
				VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
				
				if(update->sampled_image.is_depth)
				{
					layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				else
				{
					layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}
				
				VkDescriptorImageInfo *image_info = image_infos + i;
				image_info->imageView = update->sampled_image.view;
				image_info->imageLayout = layout;
				image_info->sampler = VK_NULL_HANDLE;
				
				VkWriteDescriptorSet *write = descriptor_writes + i;
				write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write->descriptorCount = 1;
				write->dstArrayElement = update->slot;
				write->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write->dstSet = graphics_device->bindless_set;
				write->dstBinding = BINDLESS_SAMPLED_IMAGE_BINDING;
				write->pImageInfo = image_info;
			}
			break;
			
			case BindlessUpdateType_Sampler:
			{
				VkDescriptorImageInfo *image_info = image_infos + i;
				image_info->imageView = VK_NULL_HANDLE;
				image_info->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				image_info->sampler = update->sampler.sampler;
				
				VkWriteDescriptorSet *write = descriptor_writes + i;
				write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write->descriptorCount = 1;
				write->dstArrayElement = update->slot;
				write->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				write->dstSet = graphics_device->bindless_set;
				write->dstBinding = BINDLESS_SAMPLER_BINDING;
				write->pImageInfo = image_info;
			}
			break;
		}
	}
	
	if(graphics_device->n_bindless_updates > 0)
	{
		vkUpdateDescriptorSets(graphics_device->device, graphics_device->n_bindless_updates, descriptor_writes, 0, 0);
		graphics_device->n_bindless_updates = 0;
	}
}

internal b32
ImageIsDepth(Image *image)
{
	return image->format == graphics_device->depth_format;
}

internal b32
ImageIsCubemap(Image *image)
{
	return image->type == VK_IMAGE_VIEW_TYPE_CUBE;
}

internal u32
ImageFaceCount(Image *image)
{
	return ImageIsCubemap(image) ? 6 : 1;
}

internal u32
ImageLayerCount(Image *image)
{
	if(image->type == VK_IMAGE_VIEW_TYPE_1D_ARRAY ||
	   image->type == VK_IMAGE_VIEW_TYPE_2D_ARRAY)
	{
		return image->depth;
	}
	
	return ImageFaceCount(image);
}

internal u32
ClampMipmapCount(u32 mipmaps, u32 w, u32 h, u32 d)
{
	return MinValue(mipmaps, 1u + (u32)(Log2F((f32)MaxValue(w, MaxValue(h, d)))));
}

internal Image
ImageAllocate(u32 width, u32 height, u32 depth,
			  VkFormat format,
			  VkImageViewType type,
			  VkImageTiling tiling,
			  u32 mipmaps,
			  VkSampleCountFlagBits samples,
			  b32 is_transient,
			  b32 is_storage)
{
	Image image = {0};
	
	image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	
	image.width = width;
	image.height = height;
	image.depth = depth;
	
	image.is_swapchain = false;
	
	image.format = format;
	image.type = type;
	image.tiling = tiling;
	
	image.mipmap_count = ClampMipmapCount(mipmaps, width, height, depth);
	image.samples = samples;
	
	if(is_transient)
	{
		image.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	else
	{
		image.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	
	if(is_storage)
	{
		image.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	
	if(ImageIsDepth(&image))
	{
		image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else
	{
		image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	
	VkImageType image_type = VK_IMAGE_TYPE_MAX_ENUM;
	
	switch(image.type)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		{
			image_type = VK_IMAGE_TYPE_1D;
		}
		break;
		
		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
		{
			image_type = VK_IMAGE_TYPE_2D;
		}
		break;
		
		case VK_IMAGE_VIEW_TYPE_3D:
		{
			image_type = VK_IMAGE_TYPE_3D;
		}
		break;
		
		default:
		{
			DebugLogCrash("Failed to find VkImageType given VkImageViewType: %d", image.type);
		}
		break;
	}
	
	VkImageCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = image_type;
	create_info.extent.width = image.width;
	create_info.extent.height = image.height;
	create_info.extent.depth = image.depth;
	create_info.mipLevels = image.mipmap_count;
	create_info.arrayLayers = ImageFaceCount(&image);
	create_info.format = image.format;
	create_info.tiling = image.tiling;
	create_info.usage = image.usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = image.samples;
	create_info.flags = 0;
	
	if(ImageIsCubemap(&image))
	{
		create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	
	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	vma_alloc_info.priority = 1.f;
	
	VK_CHECK(vmaCreateImage(graphics_device->vma_allocator, &create_info, &vma_alloc_info, &image.image, &image.allocation, &image.allocation_info),
			 "Failed to create image.");
	
	return image;
}

internal void
ImageDestroy(Image *image)
{
	vmaDestroyImage(graphics_device->vma_allocator, image->image, image->allocation);
	image->image = VK_NULL_HANDLE;
}

internal VkImageMemoryBarrier2
GetImageMemoryBarrier(Image *image, VkImageLayout layout)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	
	barrier.oldLayout = image->layout;
	barrier.newLayout = layout;
	
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	
	barrier.image = image->image;
	
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = image->mipmap_count;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = ImageLayerCount(image);
	
	// TODO(kp): This should ideally be a bit more granular.
	//           Or maybe not...
	barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;
	
	// TODO(kp): This as well.
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	
	if(layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
	   layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	
	return barrier;
}

internal void
ImageViewDestroy(ImageView *view)
{
	vkDestroyImageView(graphics_device->device, view->view, 0);
	view->view = VK_NULL_HANDLE;
}

internal ImageView
ImageViewFromImage(Image *image, u32 layer_count, u32 layer, u32 base_mip_level)
{
	VkImageViewType view_type = image->type;
	
	if(ImageIsCubemap(image) && layer_count == 1)
	{
		view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}
	
	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = image->image;
	view_create_info.viewType = view_type;
	view_create_info.format = image->format;
	
	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_create_info.subresourceRange.baseMipLevel = base_mip_level;
	view_create_info.subresourceRange.levelCount = image->mipmap_count - base_mip_level;
	view_create_info.subresourceRange.baseArrayLayer = layer;
	view_create_info.subresourceRange.layerCount = layer_count;
	
	if(ImageIsDepth(image))
	{
		// NOTE(kp): Depth AND stencil is not allowed for sampling!
		//           --> So, use depth instead.
		view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	
	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	
	ImageView view = {0};
	view.image = image;
	view.layer_count = layer_count;
	view.layer = layer;
	view.base_mip_level = base_mip_level;
	
	VK_CHECK(vkCreateImageView(graphics_device->device, &view_create_info, 0, &view.view),
			 "Failed to create texture image view.");
	
	// NOTE(kp): Swapchain images are omitted from being accessible bindlessly.
	if(!image->is_swapchain)
	{
		view.resource_id = BindlessRegisterSampledImage(view.view, ImageIsDepth(image));
	}
	
	return view;
}

internal ImageView *
FetchImageView(Image *image, u32 layer_count, u32 layer, u32 base_mip_level)
{
	u64 hash = 0;
	hash = HashBytesGenericCombine(hash,  image,          sizeof(Image));
	hash = HashBytesGenericCombine(hash, &layer_count,    sizeof(u32));
	hash = HashBytesGenericCombine(hash, &layer,          sizeof(u32));
	hash = HashBytesGenericCombine(hash, &base_mip_level, sizeof(u32));
	
	ImageView *fetched_image_view = HashTableFetchElement(&graphics_device->image_view_cache, hash);
	
	if(fetched_image_view)
	{
		return fetched_image_view;
	}
	
	ImageView view = ImageViewFromImage(image, layer_count, layer, base_mip_level);
	
	return HashTableAddElement(&graphics_device->image_view_cache, hash, &view);
}

internal ImageView *
FetchStandardImageView(Image *image)
{
	return FetchImageView(image, ImageLayerCount(image), 0, 0);
}

internal Sampler
SamplerInit(VkFilter filter,
			VkSamplerAddressMode wrap_x,
			VkSamplerAddressMode wrap_y,
			VkSamplerAddressMode wrap_z,
			VkBorderColor border_colour)
{
	VkPhysicalDeviceProperties properties = graphics_device->physical_device_properties.properties;
	
	VkSamplerCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.minFilter = filter;
	create_info.magFilter = filter;
	create_info.addressModeU = wrap_x;
	create_info.addressModeV = wrap_z;
	create_info.addressModeW = wrap_z;
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	create_info.borderColor = border_colour;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.f;
	create_info.minLod = 0.f;
	create_info.maxLod = VK_LOD_CLAMP_NONE;
	
	Sampler sampler = {0};
	sampler.filter = filter;
	sampler.wrap_x = wrap_x;
	sampler.wrap_y = wrap_y;
	sampler.wrap_z = wrap_z;
	sampler.border_colour = border_colour;
	
	VK_CHECK(vkCreateSampler(graphics_device->device, &create_info, 0, &sampler.handle),
			 "Failed to create texture sampler.");
	
	sampler.resource_id = BindlessRegisterSampler(sampler.handle);
	
	return sampler;
}

internal Sampler
SamplerInitFilter(VkFilter filter)
{
	return SamplerInit(filter,
					   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					   VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					   VK_BORDER_COLOR_INT_OPAQUE_BLACK);
}

internal void
SamplerDestroy(Sampler *sampler)
{
	vkDestroySampler(graphics_device->device, sampler->handle, 0);
	sampler->handle = VK_NULL_HANDLE;
}

internal b32
GPUBufferIsUniform(GPUBuffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) != 0;
}

internal b32
GPUBufferIsStorage(GPUBuffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) != 0;
}

internal void
GPUBufferRead(GPUBuffer *buffer, void *dst, u64 length, u64 offset)
{
	vmaCopyAllocationToMemory(graphics_device->vma_allocator, buffer->allocation, offset, dst, length);
}

internal void
GPUBufferWrite(GPUBuffer *buffer, void *src, u64 length, u64 offset)
{
	vmaCopyMemoryToAllocation(graphics_device->vma_allocator, src, buffer->allocation, offset, length);
}

internal GPUBuffer
GPUBufferAllocate(VkBufferUsageFlags usage,
				  VmaAllocationCreateFlagBits flags,
				  u64 size)
{
	GPUBuffer buffer = {0};
	
	buffer.usage = usage;
	buffer.size = size;
	buffer.allocation_flags = flags;
	
	if(GPUBufferIsStorage(&buffer))
	{
		buffer.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}
	
	VkBufferCreateInfo buffer_create_info = {0};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = buffer.size;
	buffer_create_info.usage = buffer.usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;//queueFamilyIndices.size();
	buffer_create_info.pQueueFamilyIndices = 0;//queueFamilyIndices.data();
	
	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	
	VK_CHECK(vmaCreateBuffer(graphics_device->vma_allocator, &buffer_create_info, &vma_alloc_info, &buffer.handle, &buffer.allocation, &buffer.allocation_info),
			 "Failed to create buffer.");
	
	if(GPUBufferIsStorage(&buffer))
	{
		VkBufferDeviceAddressInfo address_info = {0};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer.handle;
		
		buffer.device_address = vkGetBufferDeviceAddress(graphics_device->device, &address_info);
	}
	
	return buffer;
}

internal void
GPUBufferDestroy(GPUBuffer *buffer)
{
	vmaDestroyBuffer(graphics_device->vma_allocator, buffer->handle, buffer->allocation);
	buffer->handle = VK_NULL_HANDLE;
}

// TODO(kp): Move elsewhere.
internal String8
LoadFileBytesAndNullTerminate(MemoryArena *dst, String8 path)
{
	b8 *bytes = 0;
	
	FILE *file = fopen((char *)path.str, "rb");
	u64 file_size = 0;
	
	if(file)
	{
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		bytes = MemoryArenaPush(dst, file_size + 1);
		fread(bytes, file_size, 1, file);
		bytes[file_size] = '\0';
		
		fclose(file);
	}
	
	return String8Init(bytes, file_size);
}

internal ShaderStage
ShaderStageLoadFromBytecode(MemoryArena *arena, String8 path, VkShaderStageFlagBits type)
{
	ScratchArena scratch = GetScratch(arena);
	String8 source = LoadFileBytesAndNullTerminate(scratch.arena, path);
	
	ShaderStage stage = {0};
	stage.stage = type;
	
	VkShaderModuleCreateInfo module_create_info = {0};
	module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_create_info.codeSize = source.len;
	module_create_info.pCode = (const u32 *)source.str;
	
	VK_CHECK(vkCreateShaderModule(graphics_device->device, &module_create_info, 0, &stage.module),
			 "Failed to create shader module.");
	
	ReleaseScratch(&scratch);
	
	return stage;
}

internal void
ShaderStageDestroy(ShaderStage *stage)
{
	vkDestroyShaderModule(graphics_device->device, stage->module, 0);
	stage->module = VK_NULL_HANDLE;
}

internal ShaderProgram
ShaderProgramInit(u32 push_constant_size, u32 stage_count)
{
	ShaderProgram program = {0};
	program.push_constant_size = push_constant_size;
	program.stage_count = stage_count;
	
	return program;
}

internal void
ShaderProgramDestroy(ShaderProgram *program)
{
	for(i32 i = 0; i < program->stage_count; i++)
	{
		ShaderStageDestroy(program->stages + i);
	}
}

internal b32
ShaderProgramIsCompute(ShaderProgram *program)
{
	return program->stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT;
}

internal void
AddVertexBinding(VertexFormat *vertex, u64 stride, VkVertexInputRate input_rate)
{
	if(input_rate == VK_VERTEX_INPUT_RATE_VERTEX)
	{
		vertex->vertex_size = stride;
	}
	else if(input_rate == VK_VERTEX_INPUT_RATE_INSTANCE)
	{
		vertex->instance_size = stride;
	}
	
	VkVertexInputBindingDescription *b = vertex->bindings + vertex->binding_count;
	{
		b->binding = vertex->binding_count;
		b->stride = stride;
		b->inputRate = input_rate;
	}
	
	vertex->binding_count++;
}

internal void
AddVertexAttribute(VertexFormat *vertex, VkFormat format, u64 offset)
{
	VkVertexInputAttributeDescription *a = vertex->attributes + vertex->attribute_count;
	{
		a->binding = vertex->binding_count - 1;
		a->location = vertex->attribute_count;
		a->format = format;
		a->offset = offset;
	}
	
	vertex->attribute_count++;
}

internal BlendState
BlendStateDefault()
{
	BlendState state = {0};
	
	state.enabled = true;
	
	state.constants[0] = 0.f;
	state.constants[1] = 0.f;
	state.constants[2] = 0.f;
	state.constants[3] = 0.f;
	
	state.write_mask[0] = true;
	state.write_mask[1] = true;
	state.write_mask[2] = true;
	state.write_mask[3] = true;
	
	state.colour.op = VK_BLEND_OP_ADD;
	state.colour.src = VK_BLEND_FACTOR_ONE;
	state.colour.dst = VK_BLEND_FACTOR_ZERO;
	
	state.alpha.op = VK_BLEND_OP_ADD;
	state.alpha.src = VK_BLEND_FACTOR_ONE;
	state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	
	state.logic_op_enabled = false;
	state.logic_op = VK_LOGIC_OP_COPY;
	
	return state;
}

internal DepthStencilState
DepthStencilStateDefault()
{
	DepthStencilState state = {0};
	
	state.depth_test_enabled = true;
	state.depth_write_enabled = true;
	state.depth_compare_op = VK_COMPARE_OP_LESS;
	state.depth_bounds_test_enabled = false;
	state.stencil_test_enabled = false;
	
	return state;
}

internal VkPipelineLayout
PipelineLayoutCreate(ShaderProgram *program)
{
	VkShaderStageFlags stage = ShaderProgramIsCompute(program) ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;
	
	VkPushConstantRange push_constants = {0};
	push_constants.offset = 0;
	push_constants.size = program->push_constant_size;
	push_constants.stageFlags = stage;
	
	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &graphics_device->bindless_layout;
	
	if (push_constants.size > 0)
	{
		create_info.pushConstantRangeCount = 1;
		create_info.pPushConstantRanges = &push_constants;
	}
	else
	{
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = 0;
	}
	
	VkPipelineLayout layout = VK_NULL_HANDLE;
	
	VK_CHECK(vkCreatePipelineLayout(graphics_device->device, &create_info, 0, &layout),
			 "Failed to create pipeline layout.");
	
	return layout;
}

internal void
PipelineLayoutDestroy(VkPipelineLayout layout)
{
	vkDestroyPipelineLayout(graphics_device->device, layout, 0);
}

internal void
PipelineDestroy(VkPipeline pipeline)
{
	vkDestroyPipeline(graphics_device->device, pipeline, 0);
}

internal GraphicsPipelineDef
GraphicsPipelineDefInitDefault(ShaderProgram *program,
							   VertexFormat *vertex_format)
{
	GraphicsPipelineDef def = {0};
	
	def.program = program;
	def.vertex_format = vertex_format;
	def.cull_mode = VK_CULL_MODE_BACK_BIT;
	def.front_face = VK_FRONT_FACE_CLOCKWISE;
	def.blend_state = BlendStateDefault();
	def.depth_stencil_state = DepthStencilStateDefault();
	def.has_depth_attachment = false;
	def.samples = VK_SAMPLE_COUNT_1_BIT;
	def.min_sample_shading_enabled = 1;
	def.min_sample_shading = 0.2f;
	def.view_mask = 0;
	
	return def;
}

internal ComputePipelineDef
ComputePipelineDefInit(ShaderProgram *program)
{
	ComputePipelineDef def = {0};
	
	def.program = program;
	
	return def;
}

internal VkPipeline
GraphicsPipelineCreate(VkPipelineLayout layout,
					   GraphicsPipelineDef *definition)
{
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {0};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	if(definition->vertex_format)
	{
		vertex_input_state_create_info.vertexBindingDescriptionCount = definition->vertex_format->binding_count;
		vertex_input_state_create_info.pVertexBindingDescriptions = definition->vertex_format->bindings;
		
		vertex_input_state_create_info.vertexAttributeDescriptionCount = definition->vertex_format->attribute_count;
		vertex_input_state_create_info.pVertexAttributeDescriptions = definition->vertex_format->attributes;
	}
	else
	{
		vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
		vertex_input_state_create_info.pVertexBindingDescriptions = 0;
		
		vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
		vertex_input_state_create_info.pVertexAttributeDescriptions = 0;
	}
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {0};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
	
	VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = 0; // NOTE(kp): Using dynamic viewport.
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = 0; // NOTE(kp): Using dynamic scissor.
	
	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {0};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.f;
	rasterization_state_create_info.cullMode = definition->cull_mode;
	rasterization_state_create_info.frontFace = definition->front_face;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.f;
	rasterization_state_create_info.depthBiasClamp = 0.f;
	rasterization_state_create_info.depthBiasSlopeFactor = 0.f;
	
	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {0};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = definition->min_sample_shading_enabled;
	multisample_state_create_info.minSampleShading = definition->min_sample_shading;
	multisample_state_create_info.rasterizationSamples = definition->samples;
	multisample_state_create_info.pSampleMask = 0;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;
	
	VkPipelineColorBlendAttachmentState blend_states[MAX_COLOUR_ATTACHMENTS] = {0};
	
	VkPipelineColorBlendAttachmentState *blend_state = blend_states;
	
	for(i32 i = 0; i < definition->colour_attachment_count; i++, blend_state++)
	{
		blend_state->blendEnable = definition->blend_state.enabled;
		
		blend_state->srcColorBlendFactor = definition->blend_state.colour.src;
		blend_state->dstColorBlendFactor = definition->blend_state.colour.dst;
		blend_state->colorBlendOp = definition->blend_state.colour.op;
		
		blend_state->srcAlphaBlendFactor = definition->blend_state.alpha.src;
		blend_state->dstAlphaBlendFactor = definition->blend_state.alpha.dst;
		blend_state->alphaBlendOp = definition->blend_state.alpha.op;
		
		if(definition->blend_state.write_mask[0]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if(definition->blend_state.write_mask[1]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if(definition->blend_state.write_mask[2]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if(definition->blend_state.write_mask[3]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}
	
	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {0};
	colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_create_info.logicOpEnable = definition->blend_state.logic_op_enabled;
	colour_blend_state_create_info.logicOp = definition->blend_state.logic_op;
	colour_blend_state_create_info.attachmentCount = definition->colour_attachment_count;
	colour_blend_state_create_info.pAttachments = blend_states;
	colour_blend_state_create_info.blendConstants[0] = definition->blend_state.constants[0];
	colour_blend_state_create_info.blendConstants[1] = definition->blend_state.constants[1];
	colour_blend_state_create_info.blendConstants[2] = definition->blend_state.constants[2];
	colour_blend_state_create_info.blendConstants[3] = definition->blend_state.constants[3];
	
	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {0};
	depth_stencil_state_create_info.depthTestEnable       = definition->depth_stencil_state.depth_test_enabled;
	depth_stencil_state_create_info.depthWriteEnable      = definition->depth_stencil_state.depth_write_enabled;
	depth_stencil_state_create_info.depthCompareOp        = definition->depth_stencil_state.depth_compare_op;
	depth_stencil_state_create_info.depthBoundsTestEnable = definition->depth_stencil_state.depth_bounds_test_enabled;
	depth_stencil_state_create_info.minDepthBounds        = definition->depth_stencil_state.depth_bounds_min;
	depth_stencil_state_create_info.maxDepthBounds        = definition->depth_stencil_state.depth_bounds_max;
	depth_stencil_state_create_info.stencilTestEnable     = definition->depth_stencil_state.stencil_test_enabled;
	depth_stencil_state_create_info.front.failOp          = definition->depth_stencil_state.stencil_front.fail_op;
	depth_stencil_state_create_info.front.passOp          = definition->depth_stencil_state.stencil_front.pass_op;
	depth_stencil_state_create_info.front.depthFailOp     = definition->depth_stencil_state.stencil_front.depth_fail_op;
	depth_stencil_state_create_info.front.compareOp       = definition->depth_stencil_state.stencil_front.compare_op;
	depth_stencil_state_create_info.front.writeMask       = definition->depth_stencil_state.stencil_front.write_mask;
	depth_stencil_state_create_info.front.reference       = definition->depth_stencil_state.stencil_front.reference;
	depth_stencil_state_create_info.back.failOp           = definition->depth_stencil_state.stencil_back.fail_op;
	depth_stencil_state_create_info.back.passOp           = definition->depth_stencil_state.stencil_back.pass_op;
	depth_stencil_state_create_info.back.depthFailOp      = definition->depth_stencil_state.stencil_back.depth_fail_op;
	depth_stencil_state_create_info.back.compareOp        = definition->depth_stencil_state.stencil_back.compare_op;
	depth_stencil_state_create_info.back.writeMask        = definition->depth_stencil_state.stencil_back.write_mask;
	depth_stencil_state_create_info.back.reference        = definition->depth_stencil_state.stencil_back.reference;
	
	static VkDynamicState GRAPHICS_PIPELINE_DYNAMIC_STATES[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_BLEND_CONSTANTS // TODO(kp): Add dynamic blend constants.
	};
	
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {0};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = ArraySize(GRAPHICS_PIPELINE_DYNAMIC_STATES);
	dynamic_state_create_info.pDynamicStates = GRAPHICS_PIPELINE_DYNAMIC_STATES;
	
	VkFormat depth_stencil_format = definition->has_depth_attachment ? graphics_device->depth_format : VK_FORMAT_UNDEFINED;
	
	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {0};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.viewMask = definition->view_mask;
	pipeline_rendering_create_info.colorAttachmentCount = definition->colour_attachment_count;
	pipeline_rendering_create_info.pColorAttachmentFormats = definition->colour_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat = depth_stencil_format;
	pipeline_rendering_create_info.stencilAttachmentFormat = depth_stencil_format;
	
	VkPipelineShaderStageCreateInfo shader_stages[2] = {0};
	
	for(i32 i = 0; i < definition->program->stage_count; i++)
	{
		VkPipelineShaderStageCreateInfo *shader_stage = shader_stages + i;
		
		shader_stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage->stage = definition->program->stages[i].stage;
		shader_stage->module = definition->program->stages[i].module;
		shader_stage->pName = "main";
	}
	
	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {0};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = definition->program->stage_count;
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
	graphics_pipeline_create_info.pColorBlendState = &colour_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	graphics_pipeline_create_info.layout = layout;
	graphics_pipeline_create_info.renderPass = VK_NULL_HANDLE;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;
	graphics_pipeline_create_info.pNext = &pipeline_rendering_create_info;
	
	VkPipeline pipeline = VK_NULL_HANDLE;
	
	VK_CHECK(vkCreateGraphicsPipelines(graphics_device->device, graphics_device->pipeline_process_cache, 1, &graphics_pipeline_create_info, 0, &pipeline),
			 "Failed to create graphics pipeline.");
	
	return pipeline;
}

internal VkPipeline
ComputePipelineCreate(VkPipelineLayout layout,
					  ComputePipelineDef *definition)
{
	VkPipelineShaderStageCreateInfo shader_stage = {0};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage = definition->program->stages[0].stage;
	shader_stage.module = definition->program->stages[0].module;
	shader_stage.pName = "main";
	
	VkComputePipelineCreateInfo compute_pipeline_create_info = {0};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.stage = shader_stage;
	
	VkPipeline pipeline = VK_NULL_HANDLE;
	
	VK_CHECK(vkCreateComputePipelines(graphics_device->device, graphics_device->pipeline_process_cache, 1, &compute_pipeline_create_info, 0, &pipeline),
			 "Failed to create compute pipeline.");
	
	return pipeline;
}

internal VkPipelineLayout
FetchPipelineLayout(ShaderProgram *program)
{
	b32 is_compute = ShaderProgramIsCompute(program);
	
	u64 hash = 0;
	hash = HashBytesGenericCombine(hash, &is_compute, sizeof(b32));
	hash = HashBytesGenericCombine(hash, &program->push_constant_size, sizeof(u32));
	
	VkPipelineLayout *fetched_layout = HashTableFetchElement(&graphics_device->pipeline_layout_cache, hash);
	
	if(fetched_layout)
	{
		return *fetched_layout;
	}
	
	VkPipelineLayout layout = PipelineLayoutCreate(program);
	
	HashTableAddElement(&graphics_device->pipeline_layout_cache, hash, &layout);
	
	return layout;
}

internal PipelineState
FetchGraphicsPipeline(GraphicsPipelineDef *definition)
{
	VkPipelineLayout layout = FetchPipelineLayout(definition->program);
	
	u64 hash = 0;
	hash = HashBytesGenericCombine(hash,  definition->program,                    sizeof(ShaderProgram));
	hash = HashBytesGenericCombine(hash,  definition->vertex_format,              sizeof(VertexFormat));
	hash = HashBytesGenericCombine(hash, &definition->cull_mode,                  sizeof(VkCullModeFlags));
	hash = HashBytesGenericCombine(hash, &definition->front_face,                 sizeof(VkFrontFace));
	hash = HashBytesGenericCombine(hash, &definition->blend_state,                sizeof(BlendState));
	hash = HashBytesGenericCombine(hash, &definition->depth_stencil_state,        sizeof(DepthStencilState));
	hash = HashBytesGenericCombine(hash, &definition->colour_attachment_count,    sizeof(u32));
	hash = HashBytesGenericCombine(hash, &definition->colour_attachment_formats,  sizeof(VkFormat) * MAX_COLOUR_ATTACHMENTS);
	hash = HashBytesGenericCombine(hash, &definition->has_depth_attachment,       sizeof(b32));
	hash = HashBytesGenericCombine(hash, &definition->samples,                    sizeof(VkSampleCountFlagBits));
	hash = HashBytesGenericCombine(hash, &definition->min_sample_shading_enabled, sizeof(b32));
	hash = HashBytesGenericCombine(hash, &definition->min_sample_shading,         sizeof(f32));
	hash = HashBytesGenericCombine(hash, &definition->view_mask,                  sizeof(u32));
	
	VkPipeline *fetched_pipeline = HashTableFetchElement(&graphics_device->pipeline_cache, hash);
	
	if(fetched_pipeline)
	{
		PipelineState st = {0};
		st.pipeline = *fetched_pipeline;
		st.layout = layout;
		
		return st;
	}
	
	PipelineState st = {0};
	st.pipeline = GraphicsPipelineCreate(layout, definition);
	st.layout = layout;
	
	HashTableAddElement(&graphics_device->pipeline_cache, hash, &st.pipeline);
	
	return st;
}

internal PipelineState
FetchComputePipeline(ComputePipelineDef *definition)
{
	VkPipelineLayout layout = FetchPipelineLayout(definition->program);
	
	u64 hash = HashBytesGeneric(definition->program, sizeof(ShaderProgram));
	
	VkPipeline *fetched_pipeline = HashTableFetchElement(&graphics_device->pipeline_cache, hash);
	
	if(fetched_pipeline)
	{
		PipelineState st = {0};
		st.pipeline = *fetched_pipeline;
		st.layout = layout;
		
		return st;
	}
	
	PipelineState st = {0};
	st.pipeline = ComputePipelineCreate(layout, definition);
	st.layout = layout;
	
	HashTableAddElement(&graphics_device->pipeline_cache, hash, &st.pipeline);
	
	return st;
}

internal void
SwapchainInit(Swapchain *swapchain, MemoryArena *arena, Platform *platform)
{
	ScratchArena scratch = GetScratch(arena);
	
	SwapchainSupportDetails details = QuerySwapchainSupport(scratch.arena, graphics_device->physical_device, graphics_device->surface);
	
	VkSurfaceFormatKHR surface_format = GraphicsChooseSwapSurfaceFormat(details.surface_formats, details.surface_format_count);
	VkPresentModeKHR present_mode = GraphicsChooseSwapPresentMode(details.present_modes, details.present_mode_count, true);
	VkExtent2D extent = GraphicsChooseSwapExtent(platform, &details.capabilities);
	
	swapchain->width = extent.width;
	swapchain->height = extent.height;
	swapchain->format = surface_format.format;
	
	u32 image_count = details.capabilities.minImageCount + 1;
	
	if(details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount)
	{
		image_count = details.capabilities.maxImageCount;
	}
	
	VkSwapchainCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = graphics_device->surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = 0;
	create_info.preTransform = details.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	
	VK_CHECK(vkCreateSwapchainKHR(graphics_device->device, &create_info, 0, &swapchain->handle),
			 "Failed to create swapchain.");
	
	vkGetSwapchainImagesKHR(graphics_device->device, swapchain->handle, &image_count, 0);
	
	if(image_count <= 0)
	{
		DebugLogCrash("Failed to find any images in swapchain.");
	}
	
	swapchain->swapchain_image_count = image_count;
	
	swapchain->swapchain_images      = MemoryArenaPush(arena, sizeof(Image)     * image_count);
	swapchain->swapchain_image_views = MemoryArenaPush(arena, sizeof(ImageView) * image_count);
	
	VkImage *vk_images = MemoryArenaPush(scratch.arena, sizeof(VkImage) * image_count);
	
	vkGetSwapchainImagesKHR(graphics_device->device, swapchain->handle, &image_count, vk_images);
	
	for(i32 i = 0; i < image_count; i++)
	{
		Image *image = swapchain->swapchain_images + i;
		
		image->image = vk_images[i];
		image->layout = VK_IMAGE_LAYOUT_UNDEFINED;
		
		image->width = swapchain->width;
		image->height = swapchain->height;
		image->depth = 1;
		
		image->is_swapchain = true;
		
		image->format = swapchain->format;
		image->type = VK_IMAGE_VIEW_TYPE_2D;
		image->tiling = VK_IMAGE_TILING_OPTIMAL;
		
		image->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		
		image->mipmap_count = 1;
		image->samples = VK_SAMPLE_COUNT_1_BIT;
		
		swapchain->swapchain_image_views[i] = ImageViewFromImage(image, ImageLayerCount(image), 0, 0);
	}
	
	ReleaseScratch(&scratch);
	
	DebugLog("Swapchain created.");
}

internal void
SwapchainDestroy(Swapchain *swapchain)
{
	for(i32 i = 0; i < swapchain->swapchain_image_count; i++)
	{
		ImageViewDestroy(swapchain->swapchain_image_views + i);
	}
	
	vkDestroySwapchainKHR(graphics_device->device, swapchain->handle, 0);
}

internal void
SwapchainAcquireNextImage(Swapchain *swapchain)
{
	VkAcquireNextImageInfoKHR info = {0};
	info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	info.swapchain = swapchain->handle;
	info.timeout = UINT64_MAX;
	info.semaphore = GetCurrentImageAvailableSemaphore();
	info.fence = VK_NULL_HANDLE;
	info.deviceMask = 1;
	
	VkResult result = vkAcquireNextImage2KHR(graphics_device->device, &info, &swapchain->current_image_index);
	
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		DebugLogCrash("TODO We need to rebuild the entire swapchain here.");
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		DebugLogCrash("Failed to acquire next image in swapchain.");
	}
}

internal Image *
GetCurrentSwapchainImage(Swapchain *swapchain)
{
	return &swapchain->swapchain_images[swapchain->current_image_index];
}

internal ImageView *
GetCurrentSwapchainImageView(Swapchain *swapchain)
{
	return &swapchain->swapchain_image_views[swapchain->current_image_index];
}

internal void
CmdBegin(CommandBuffer *cmd)
{
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	
	VK_CHECK(vkBeginCommandBuffer(cmd->handle, &begin_info),
			 "Failed to begin recording instant command buffer.");
}

internal void
CmdEnd(CommandBuffer *cmd)
{
	VK_CHECK(vkEndCommandBuffer(cmd->handle),
			 "Failed to record command buffer.");
}

internal void
CmdSetViewport(CommandBuffer *cmd,
			   VkViewport viewport)
{
	// NOTE(kp): Vulkan uses a Y+ down coordinate system but
	//           we use Y+ up, so we flip the viewport internally
	//           to account for this.
	
	VkViewport corrected_viewport = {0};
	corrected_viewport.x = viewport.x;
	corrected_viewport.y = viewport.height + viewport.y;
	corrected_viewport.width = viewport.width;
	corrected_viewport.height = -viewport.height;
	corrected_viewport.minDepth = viewport.minDepth;
	corrected_viewport.maxDepth = viewport.maxDepth;
	
	vkCmdSetViewport(cmd->handle, 0, 1, &corrected_viewport);
}

internal void
CmdSetScissor(CommandBuffer *cmd,
			  VkRect2D scissor)
{
	vkCmdSetScissor(cmd->handle, 0, 1, &scissor);
}

internal void
CmdBeginRendering(CommandBuffer *cmd, RenderInfo *info)
{
	VkRenderingInfo rendering_info = {0};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	rendering_info.renderArea.offset = (VkOffset2D){ 0, 0 };
	rendering_info.renderArea.extent = (VkExtent2D){ info->width, info->height };
	rendering_info.layerCount = 1;
	rendering_info.viewMask = info->view_mask;
	rendering_info.colorAttachmentCount = info->colour_attachment_count;
	rendering_info.pColorAttachments = info->colour_attachments;
	rendering_info.pDepthAttachment = info->depth_attachment.imageView ? &info->depth_attachment : 0;
	rendering_info.pStencilAttachment = 0;
	
	vkCmdBeginRendering(cmd->handle, &rendering_info);
	
	VkViewport viewport = {0};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = (f32)info->width;
	viewport.height = (f32)info->height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	
	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D){ 0, 0 };
	scissor.extent = (VkExtent2D){ info->width, info->height };
	
	// NOTE(kp): We have to update the dynamic states
	//           every time we draw something.
	CmdSetViewport(cmd, viewport);
	CmdSetScissor(cmd, scissor);
}

internal void
CmdEndRendering(CommandBuffer *cmd)
{
	vkCmdEndRendering(cmd->handle);
}

internal void
CmdPipelineBarrier(CommandBuffer *cmd,
				   VkDependencyFlags dependency_flags,
				   u32 memory_barrier_count,
				   VkMemoryBarrier2 *memory_barriers,
				   u32 buffer_memory_barrier_count,
				   VkBufferMemoryBarrier2 *buffer_memory_barriers,
				   u32 image_memory_barrier_count,
				   VkImageMemoryBarrier2 *image_memory_barriers)
{
	VkDependencyInfo dependency = {0};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.dependencyFlags = dependency_flags;
	
	dependency.memoryBarrierCount = memory_barrier_count;
	dependency.pMemoryBarriers = memory_barriers;
	
	dependency.bufferMemoryBarrierCount = buffer_memory_barrier_count;
	dependency.pBufferMemoryBarriers = buffer_memory_barriers;
	
	dependency.imageMemoryBarrierCount = image_memory_barrier_count;
	dependency.pImageMemoryBarriers = image_memory_barriers;
	
	vkCmdPipelineBarrier2(cmd->handle, &dependency);
}

internal void
CmdBindDescriptors(CommandBuffer *cmd,
				   VkPipelineBindPoint bind_point,
				   VkPipelineLayout layout,
				   u32 first,
				   u32 descriptor_count,
				   VkDescriptorSet *descriptors,
				   u32 dynamic_offset_count,
				   u32 *dynamic_offsets)
{
	vkCmdBindDescriptorSets(cmd->handle,
							bind_point,
							layout,
							first,
							descriptor_count,
							descriptors,
							dynamic_offset_count,
							dynamic_offsets);
}

internal void
CmdBindBindless(CommandBuffer *cmd,
				VkPipelineBindPoint bind_point,
				VkPipelineLayout layout)
{
	CmdBindDescriptors(cmd,
					   bind_point,
					   layout,
					   0,
					   1, &graphics_device->bindless_set,
					   0, 0);
}

internal void
CmdBindPipeline(CommandBuffer *cmd,
				VkPipelineBindPoint bind_point,
				VkPipeline pipeline)
{
	vkCmdBindPipeline(cmd->handle,
					  bind_point,
					  pipeline);
}

internal void
CmdBindVertexBuffer(CommandBuffer *cmd,
					u32 binding,
					GPUBuffer *buffer,
					u64 offset)
{
	vkCmdBindVertexBuffers(cmd->handle,
						   binding,
						   1, &buffer->handle,
						   &offset);
}

internal void
CmdBindIndexBuffer(CommandBuffer *cmd,
				   GPUBuffer *buffer,
				   u64 offset)
{
	vkCmdBindIndexBuffer(cmd->handle,
						 buffer->handle,
						 offset,
						 VK_INDEX_TYPE_UINT16);
}

internal void
CmdPushConstants(CommandBuffer *cmd,
				 VkPipelineLayout layout,
				 VkShaderStageFlags shader_stage,
				 u32 size,
				 void *data,
				 u32 offset)
{
	vkCmdPushConstants(cmd->handle,
					   layout,
					   shader_stage,
					   offset,
					   size,
					   data);
}

internal void
CmdDrawIndexed(CommandBuffer *cmd,
			   u32 index_count,
			   u32 instance_count,
			   u32 first_index,
			   i32 vertex_offset,
			   u32 first_instance)
{
	vkCmdDrawIndexed(cmd->handle,
					 index_count,
					 instance_count,
					 first_index,
					 vertex_offset,
					 first_instance);
}

internal void
CmdBlitImage(CommandBuffer *cmd,
			 Image *src, VkImageLayout src_layout,
			 Image *dst, VkImageLayout dst_layout,
			 u32 region_count, VkImageBlit *regions,
			 VkFilter filter)
{
	vkCmdBlitImage(cmd->handle,
				   src->image, src_layout,
				   dst->image, dst_layout,
				   region_count, regions,
				   filter);
}

internal void
CmdTransitionImageLayout(CommandBuffer *cmd,
						 Image *image,
						 VkImageLayout layout)
{
	VkImageMemoryBarrier2 barrier = GetImageMemoryBarrier(image, layout);
	
	CmdPipelineBarrier(cmd, 0,
					   0, 0,
					   0, 0,
					   1, &barrier);
	
	image->layout = layout;
}

internal void
CmdPrepareForMipmapping(CommandBuffer *cmd,
						Image *image)
{
	CmdTransitionImageLayout(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
}

internal void
CmdGenerateMipmaps(CommandBuffer *cmd, Image *image)
{
	Assert(image->layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && "Image must be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.");
	
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = image->image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = ImageLayerCount(image);
	barrier.subresourceRange.levelCount = 1;
	
	for(i32 i = 1; i < image->mipmap_count; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		
		CmdPipelineBarrier(cmd,
						   0,
						   0, 0,
						   0, 0,
						   1, &barrier);
		
		for(i32 face = 0; face < ImageFaceCount(image); face++)
		{
			i32 src_mip_width  = (i32)image->width  >> (i - 1);
			i32 src_mip_height = (i32)image->height >> (i - 1);
			i32 dst_mip_width  = (i32)image->width  >> (i - 0);
			i32 dst_mip_height = (i32)image->height >> (i - 0);
			
			VkImageBlit blit = {0};
			
			blit.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
			blit.srcOffsets[1] = (VkOffset3D){ src_mip_width, src_mip_height, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;
			
			blit.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
			blit.dstOffsets[1] = (VkOffset3D){ dst_mip_width, dst_mip_height, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;
			
			CmdBlitImage(cmd,
						 image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						 image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						 1, &blit,
						 VK_FILTER_LINEAR);
		}
		
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		
		CmdPipelineBarrier(cmd,
						   0,
						   0, 0,
						   0, 0,
						   1, &barrier);
	}
	
	barrier.subresourceRange.baseMipLevel = image->mipmap_count - 1;
	
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	
	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	
	CmdPipelineBarrier(cmd,
					   0,
					   0, 0,
					   0, 0,
					   1, &barrier);
	
	image->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

internal void
CmdCopyBufferToBuffer(CommandBuffer *cmd,
					  GPUBuffer *src,
					  GPUBuffer *dst,
					  u32 region_count,
					  VkBufferCopy *regions)
{
	vkCmdCopyBuffer(cmd->handle,
					src->handle,
					dst->handle,
					region_count,
					regions);
}

internal void
CmdCopyBufferToImageMultiRegion(CommandBuffer *cmd,
								GPUBuffer *buffer,
								Image *image,
								u32 region_count,
								VkBufferImageCopy *regions)
{
	Assert(image->layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && "Image must be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.");
	
	vkCmdCopyBufferToImage(cmd->handle,
						   buffer->handle,
						   image->image,
						   image->layout,
						   region_count,
						   regions);
}

internal void
CmdCopyBufferToImage(CommandBuffer *cmd,
					 GPUBuffer *buffer,
					 Image *image)
{
	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = (VkOffset3D){ 0, 0, 0 };
	region.imageExtent = (VkExtent3D){ image->width, image->height, 1 };
	
	CmdCopyBufferToImageMultiRegion(cmd, buffer, image, 1, &region);
}

internal void
CmdDispatch(CommandBuffer *cmd,
			u32 x, u32 y, u32 z)
{
	vkCmdDispatch(cmd->handle,
				  x, y, z);
}

internal void
CommandPoolInit(CommandPool *pool,
				u32 family_index)
{
	VkCommandPoolCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;
	
	VK_CHECK(vkCreateCommandPool(graphics_device->device, &create_info, 0, &pool->handle),
			 "Failed to create command pool.");
	
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {0};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = ArraySize(pool->free_buffers);
	command_buffer_allocate_info.commandPool = pool->handle;
	
	VK_CHECK(vkAllocateCommandBuffers(graphics_device->device, &command_buffer_allocate_info, pool->free_buffers),
			 "Failed to allocate command pool command buffers.");
}

internal void
CommandPoolDestroy(CommandPool *pool)
{
	vkDestroyCommandPool(graphics_device->device, pool->handle, 0);
	pool->handle = VK_NULL_HANDLE;
}

internal CommandBuffer
FetchFreeCommandBuffer(CommandPool *pool)
{
	CommandBuffer cmd = {0};
	cmd.handle = pool->free_buffers[pool->free_index++];
	
	return cmd;
}

internal void
CommandPoolReset(CommandPool *pool)
{
	vkResetCommandPool(graphics_device->device, pool->handle, 0);
	pool->free_index = 0;
}

global const char *GRAPHICS_VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation"
};

internal b32
CheckGraphicsPhysicalDeviceExtensionSupport(MemoryArena *arena, VkPhysicalDevice physical_device)
{
	u32 extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, 0, &extension_count, 0);
	
	if(extension_count <= 0)
	{
		DebugLogCrash("Failed to find any device extension properties.");
	}
	
	ScratchArena scratch = GetScratch(arena);
	
	VkExtensionProperties *available_exts = MemoryArenaPush(scratch.arena, sizeof(VkExtensionProperties) * extension_count);
	vkEnumerateDeviceExtensionProperties(physical_device, 0, &extension_count, available_exts);
	
	for(i32 i = 0; i < extension_count; i++)
	{
		for(i32 j = 0; j < ArraySize(GRAPHICS_VALIDATION_LAYERS); j++)
		{
			if(CStringCompare(available_exts[i].extensionName, GRAPHICS_VALIDATION_LAYERS[j]) == 0)
			{
				ReleaseScratch(&scratch);
				return false;
			}
		}
	}
	
	ReleaseScratch(&scratch);
	return true;
}

// TODO(kp): Frankly this whole function fucking sucks.
internal u32
AssignGraphicsPhysicalDeviceUsability(MemoryArena *arena,
									  VkSurfaceKHR surface,
									  VkPhysicalDevice physical_device,
									  VkPhysicalDeviceProperties2 properties,
									  VkPhysicalDeviceFeatures2 features,
									  b32 *has_essentials)
{
	u32 usability = 0;
	
	b32 adequate_swap_chain = false;
	b32 has_required_extensions = CheckGraphicsPhysicalDeviceExtensionSupport(arena, physical_device);
	b32 has_anisotropy = features.features.samplerAnisotropy;
	
	// NOTE(kp): Prefer / give more weight to discrete gpus than integrated gpus.
	if(properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		usability += 4;
	}
	else if(properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		usability += 1;
	}
	
	// NOTE(kp): If we have anisotropy then that's good... I guess :))).
	if(has_anisotropy)
	{
		usability += 1;
	}
	
	// NOTE(kp): It must have the required extensions.
	if(has_required_extensions)
	{
		ScratchArena scratch = GetScratch(arena);
		
		SwapchainSupportDetails details = QuerySwapchainSupport(scratch.arena, physical_device, surface);
		
		adequate_swap_chain = ((details.surface_format_count > 0) &&
							   (details.present_mode_count > 0));
		
		usability += 3;
		
		ReleaseScratch(&scratch);
	}
	
	// NOTE(kp): Essential features must be satisfied.
	if(has_essentials)
	{
		(*has_essentials) = has_required_extensions && adequate_swap_chain && has_anisotropy;
	}
	
	return usability;
}

internal b32
CheckForValidationLayerSupport(MemoryArena *arena)
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);
	
	ScratchArena scratch = GetScratch(arena);
	
	VkLayerProperties *available_layers = MemoryArenaPush(scratch.arena, sizeof(VkLayerProperties) * layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);
	
	for(i32 i = 0; i < ArraySize(GRAPHICS_VALIDATION_LAYERS); i++)
	{
		b32 has_layer = false;
		const char *layer_name_0 = GRAPHICS_VALIDATION_LAYERS[i];
		
		for(i32 j = 0; j < layer_count; j++)
		{
			const char *layer_name_1 = available_layers[j].layerName;
			
			if(CStringCompare(layer_name_0, layer_name_1) == 0)
			{
				has_layer = true;
				break;
			}
		}
		
		if(!has_layer)
		{
			ReleaseScratch(&scratch);
			return 0;
		}
	}
	
	ReleaseScratch(&scratch);
	return 1;
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL
GraphicsVulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
							VkDebugUtilsMessageTypeFlagsEXT message_type,
							const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
							void *p_user_data)
{
	if(message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		DebugLogCrash("Severity=%d, Type=%d, Message=\"%s\"", message_severity, message_type, p_callback_data->pMessage);
	}
	
	return VK_FALSE;
}

internal VkResult
CreateGraphicsDeviceDebugUtilsMessengerExt(VkInstance instance,
										   VkDebugUtilsMessengerCreateInfoEXT *debug_info,
										   const VkAllocationCallbacks *allocator,
										   VkDebugUtilsMessengerEXT *messenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	
	if(fn)
	{
		return fn(instance, debug_info, allocator, messenger);
	}
	
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// TODO(kp): We really shouldn't need to initialize
//           and de-initialize volk like this. Surely
//           we should load all fpointers into a table
//           which we store in program memory (possible
//           in volk) and just load that table back
//           in after reloading?

// TODO(kp): Also I'm 99% certain that VMA's function
//           pointers also break down here, and I'm
//           not setting them back... Need to test.

internal void
GraphicsDeviceBeforeHotReload()
{
	volkFinalize();
}

internal void
GraphicsDeviceAfterHotReload()
{
	volkInitialize();
	volkLoadInstance(graphics_device->instance);
	volkLoadDevice(graphics_device->device);
	
	GraphicsWaitIdle();
}

internal void
GraphicsDeviceInit(Platform *platform, MemoryArena *arena)
{
	VkApplicationInfo core_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = WINDOW_TITLE,
		.applicationVersion = VK_MAKE_API_VERSION(APP_VERSION_VARIANT,
												  APP_VERSION_MAJOR,
												  APP_VERSION_MINOR,
												  APP_VERSION_PATCH),
		.pEngineName = ENGINE_NAME,
		.engineVersion = VK_MAKE_API_VERSION(ENGINE_VERSION_VARIANT,
											 ENGINE_VERSION_MAJOR,
											 ENGINE_VERSION_MINOR,
											 ENGINE_VERSION_PATCH),
		.apiVersion = VK_API_VERSION_1_4
	};
	
	VkInstanceCreateInfo instance_create_info = {0};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &core_info;
	
	volkInitialize();
	
	ScratchArena scratch = GetScratch(arena);
	
	instance_create_info.ppEnabledExtensionNames = GetInstanceExtensions(scratch.arena, platform, &instance_create_info.enabledExtensionCount);
	
	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
	debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	
	debug_create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	
	debug_create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	
	debug_create_info.pfnUserCallback = GraphicsVulkanDebugCallback;
	debug_create_info.pUserData = 0;
	
	graphics_device->has_validation_layers = CheckForValidationLayerSupport(scratch.arena);
	
	if(graphics_device->has_validation_layers)
	{
		DebugLog("Validation layer support verified.");
		
		instance_create_info.enabledLayerCount = ArraySize(GRAPHICS_VALIDATION_LAYERS);
		instance_create_info.ppEnabledLayerNames = GRAPHICS_VALIDATION_LAYERS;
		instance_create_info.pNext = &debug_create_info;
	}
	else
	{
		DebugLog("No validation layer support.");
		
		instance_create_info.enabledLayerCount = 0;
		instance_create_info.ppEnabledLayerNames = 0;
		instance_create_info.pNext = 0;
	}
	
#ifdef __APPLE__
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
	
	VK_CHECK(vkCreateInstance(&instance_create_info, 0, &graphics_device->instance),
			 "Failed to create instance.");
	
	volkLoadInstance(graphics_device->instance);
	
	if(graphics_device->has_validation_layers)
	{
		VK_CHECK(CreateGraphicsDeviceDebugUtilsMessengerExt(graphics_device->instance,
															&debug_create_info,
															0,
															&graphics_device->debug_messenger),
				 "Failed to create debug messenger.");
	}
	
	if(!platform->CreateVulkanSurface((void *)graphics_device->instance, (void *)&graphics_device->surface))
	{
		DebugLogCrash("Failed to create surface.");
	}
	
	// NOTE(kp): Enumerate physical devices.
	{
		u32 device_count = 0;
		vkEnumeratePhysicalDevices(graphics_device->instance, &device_count, 0);
		
		if(device_count <= 0)
		{
			DebugLogCrash("Failed to find GPUs with Vulkan support.");
		}
		
		VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		VkPhysicalDeviceFeatures2   features   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2   };
		
		VkPhysicalDevice *devices = MemoryArenaPush(scratch.arena, sizeof(VkPhysicalDevice) * device_count);
		vkEnumeratePhysicalDevices(graphics_device->instance, &device_count, devices);
		
		vkGetPhysicalDeviceProperties2(devices[0], &properties);
		vkGetPhysicalDeviceFeatures2(devices[0], &features);
		
		graphics_device->physical_device = devices[0];
		graphics_device->physical_device_properties = properties;
		graphics_device->physical_device_features = features;
		
		b32 has_essentials = false;
		
		u32 usability0 = AssignGraphicsPhysicalDeviceUsability(scratch.arena,
															   graphics_device->surface,
															   graphics_device->physical_device,
															   properties,
															   features,
															   &has_essentials);
		
		u32 selected_index = 0;
		
		for(i32 i = 0; i < device_count; i++)
		{
			vkGetPhysicalDeviceProperties2(devices[i], &graphics_device->physical_device_properties);
			vkGetPhysicalDeviceFeatures2(devices[i], &graphics_device->physical_device_features);
			
			u32 usability1 = AssignGraphicsPhysicalDeviceUsability(scratch.arena,
																   graphics_device->surface,
																   devices[i],
																   properties,
																   features,
																   &has_essentials);
			
			if(usability1 > usability0 && has_essentials)
			{
				usability0 = usability1;
				
				graphics_device->physical_device = devices[i];
				graphics_device->physical_device_properties = properties;
				graphics_device->physical_device_features = features;
				
				selected_index = i;
			}
			
			if(!graphics_device->physical_device)
			{
				DebugLogCrash("Unable to find a suitable GPU.");
			}
			
			DebugLog("Selected a suitable GPU: %d", selected_index);
		}
	}
	
	graphics_device->max_msaa_samples = FindGraphicsMaxUsableSampleCount(graphics_device->physical_device_properties);
	graphics_device->depth_format = FindGraphicsDepthFormat(graphics_device->physical_device);
	
	// NOTE(kp): Locate the graphics queue.
	{
		u32 queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(graphics_device->physical_device, &queue_family_count, 0);
		
		if(queue_family_count <= 0)
		{
			DebugLogCrash("Failed to find any queue families.");
		}
		
		VkQueueFamilyProperties *queue_families = MemoryArenaPush(scratch.arena, sizeof(VkQueueFamilyProperties) * queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(graphics_device->physical_device, &queue_family_count, queue_families);
		
		for(i32 i = 0; i < queue_family_count; i++)
		{
			if((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				b32 present_support = false;
				
				vkGetPhysicalDeviceSurfaceSupportKHR(graphics_device->physical_device,
													 i,
													 graphics_device->surface,
													 &present_support);
				
				if(present_support)
				{
					graphics_device->graphics_queue_family_index = i;
					break;
				}
				
				continue;
			}
		}
	}
	
	f32 queue_priority = 1.f;
	
	VkDeviceQueueCreateInfo graphics_queue_create_info = {0};
	graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphics_queue_create_info.queueFamilyIndex = graphics_device->graphics_queue_family_index;
	graphics_queue_create_info.queueCount = 1;
	graphics_queue_create_info.pQueuePriorities = &queue_priority;
	
	graphics_device->physical_device_features.features.robustBufferAccess = VK_FALSE;
	
	VkPhysicalDeviceVulkan11Features vulkan11_features = {0};
	vulkan11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	vulkan11_features.shaderDrawParameters = VK_TRUE;
	vulkan11_features.multiview = VK_TRUE;
	
	VkPhysicalDeviceVulkan12Features vulkan12_features = {0};
	vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12_features.runtimeDescriptorArray = VK_TRUE;
	vulkan12_features.descriptorBindingPartiallyBound = VK_TRUE;
	vulkan12_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
	vulkan12_features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	vulkan12_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	vulkan12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	vulkan12_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	vulkan12_features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	vulkan12_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	vulkan12_features.bufferDeviceAddress = VK_TRUE;
	vulkan12_features.pNext = &vulkan11_features;
	
	VkPhysicalDeviceVulkan13Features vulkan13_features = {0};
	vulkan13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13_features.dynamicRendering = VK_TRUE;
	vulkan13_features.synchronization2 = VK_TRUE;
	vulkan13_features.pNext = &vulkan12_features;
	
	static const char *GRAPHICS_DEVICE_EXTENSIONS[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
#ifdef __APPLE__
		"VK_KHR_portability_subset"
#endif
	};
	
	VkDeviceCreateInfo device_create_info = {0};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = 0;
	device_create_info.enabledExtensionCount = ArraySize(GRAPHICS_DEVICE_EXTENSIONS);
	device_create_info.ppEnabledExtensionNames = GRAPHICS_DEVICE_EXTENSIONS;
	device_create_info.pEnabledFeatures = &graphics_device->physical_device_features.features;
	device_create_info.pNext = &vulkan13_features;
	
	if(graphics_device->has_validation_layers)
	{
		device_create_info.enabledLayerCount = ArraySize(GRAPHICS_VALIDATION_LAYERS);
		device_create_info.ppEnabledLayerNames = GRAPHICS_VALIDATION_LAYERS;
		
		DebugLog("Enabled validation layers.");
	}
	
	VK_CHECK(vkCreateDevice(graphics_device->physical_device, &device_create_info, 0, &graphics_device->device),
			 "Failed to create logical device.");
	
	vkGetDeviceQueue(graphics_device->device,
					 graphics_device->graphics_queue_family_index,
					 0,
					 &graphics_device->graphics_queue);
	
	DebugLog("Created logical device.");
	
	VkSemaphoreCreateInfo semaphore_create_info = {0};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	for(i32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		CommandPoolInit(&graphics_device->frames[i].command_pool, graphics_device->graphics_queue_family_index);
		
		VkFenceCreateInfo in_flight_fence_create_info = {0};
		in_flight_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		in_flight_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		
		VK_CHECK(vkCreateFence(graphics_device->device, &in_flight_fence_create_info, 0, &graphics_device->frames[i].in_flight_fence),
				 "Failed to create queue frame in flight fence.");
		
		VkFenceCreateInfo instant_submit_fence_create_info = {0};
		instant_submit_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		instant_submit_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		
		VK_CHECK(vkCreateFence(graphics_device->device, &instant_submit_fence_create_info, 0, &graphics_device->frames[i].instant_submit_fence),
				 "Failed to create queue frame instant submit fence.");
		
		VK_CHECK(vkCreateSemaphore(graphics_device->device, &semaphore_create_info, 0, &graphics_device->frames[i].image_available_semaphore),
				 "Failed to create image available semaphore.");
		
		VK_CHECK(vkCreateSemaphore(graphics_device->device, &semaphore_create_info, 0, &graphics_device->frames[i].render_finished_semaphore),
				 "Failed to create render finished semaphore.");
	}
	
	DebugLog("Created frame sync objects.");
	
	u32 version = 0;
	VkResult result = vkEnumerateInstanceVersion(&version);
	
	if(result == VK_SUCCESS)
	{
		u32 major = VK_API_VERSION_MAJOR(version);
		u32 minor = VK_API_VERSION_MINOR(version);
		
		DebugLog("Using Vulkan %d.%d", major, minor);
	}
	else
	{
		DebugLog("Failed to retrieve Vulkan version.");
	}
	
	volkLoadDevice(graphics_device->device);
	
	VmaVulkanFunctions vulkan_functions = {0};
	vulkan_functions.vkAllocateMemory = vkAllocateMemory;
	vulkan_functions.vkBindBufferMemory = vkBindBufferMemory;
	vulkan_functions.vkBindImageMemory = vkBindImageMemory;
	vulkan_functions.vkCreateBuffer = vkCreateBuffer;
	vulkan_functions.vkCreateImage = vkCreateImage;
	vulkan_functions.vkDestroyBuffer = vkDestroyBuffer;
	vulkan_functions.vkDestroyImage = vkDestroyImage;
	vulkan_functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vulkan_functions.vkFreeMemory = vkFreeMemory;
	vulkan_functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vulkan_functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vulkan_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vulkan_functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vulkan_functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vulkan_functions.vkMapMemory = vkMapMemory;
	vulkan_functions.vkUnmapMemory = vkUnmapMemory;
	vulkan_functions.vkCmdCopyBuffer = vkCmdCopyBuffer;
	
	VmaAllocatorCreateInfo allocator_create_info = {0};
	allocator_create_info.physicalDevice = graphics_device->physical_device;
	allocator_create_info.device = graphics_device->device;
	allocator_create_info.instance = graphics_device->instance;
	allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	
	//vmaImportVulkanFunctionsFromVolk(&allocator_create_info, &vulkan_functions);
	
	allocator_create_info.pVulkanFunctions = &vulkan_functions;
	
	VK_CHECK(vmaCreateAllocator(&allocator_create_info, &graphics_device->vma_allocator),
			 "Failed to create Vulkan Memory Allocator.");
	
	DebugLog("Created Vulkan Memory Allocator.");
	
	VkPipelineCacheCreateInfo pipeline_cache_create_info = {0};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = 0;
	pipeline_cache_create_info.flags = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = 0;
	
	VK_CHECK(vkCreatePipelineCache(graphics_device->device, &pipeline_cache_create_info, 0, &graphics_device->pipeline_process_cache),
			 "Failed to process pipeline cache.");
	
	DebugLog("Created graphics pipeline process cache.");
	
	SwapchainInit(&graphics_device->swapchain, arena, platform);
	BindlessInit();
	
	HashTableInit(&graphics_device->image_view_cache, arena, sizeof(ImageView));
	HashTableInit(&graphics_device->pipeline_cache, arena, sizeof(VkPipeline));
	HashTableInit(&graphics_device->pipeline_layout_cache, arena, sizeof(VkPipelineLayout));
	
	DebugLog("Created graphics object caches.");
	
	ReleaseScratch(&scratch);
}

internal void
GraphicsDeviceDestroy()
{
	GraphicsWaitIdle();
	
	// NOTE(kp): Destroy cached image views.
	for(i32 i = 0; i < ArraySize(graphics_device->image_view_cache.buckets); i++)
	{
		if(graphics_device->image_view_cache.buckets[i])
		{
			HashTableNode *node = graphics_device->image_view_cache.buckets[i];
			
			while(node)
			{
				ImageViewDestroy((ImageView *)node->data);
				node = node->next;
			}
		}
	}
	
	// NOTE(kp): Destroy cached pipeline layouts.
	for(i32 i = 0; i < ArraySize(graphics_device->pipeline_layout_cache.buckets); i++)
	{
		if(graphics_device->pipeline_layout_cache.buckets[i])
		{
			HashTableNode *node = graphics_device->pipeline_layout_cache.buckets[i];
			
			while(node)
			{
				PipelineLayoutDestroy(*((VkPipelineLayout *)node->data));
				node = node->next;
			}
		}
	}
	
	// NOTE(kp): Destroy cached pipelines.
	for(i32 i = 0; i < ArraySize(graphics_device->pipeline_cache.buckets); i++)
	{
		if(graphics_device->pipeline_cache.buckets[i])
		{
			HashTableNode *node = graphics_device->pipeline_cache.buckets[i];
			
			while(node)
			{
				PipelineDestroy(*((VkPipeline *)node->data));
				node = node->next;
			}
		}
	}
	
	// NOTE(kp): Clean up frame synchronization objects.
	for(i32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		CommandPoolDestroy(&graphics_device->frames[i].command_pool);
		
		vkDestroyFence(graphics_device->device, graphics_device->frames[i].in_flight_fence, 0);
		vkDestroyFence(graphics_device->device, graphics_device->frames[i].instant_submit_fence, 0);
		
		vkDestroySemaphore(graphics_device->device, graphics_device->frames[i].render_finished_semaphore, 0);
		vkDestroySemaphore(graphics_device->device, graphics_device->frames[i].image_available_semaphore, 0);
	}
	
	BindlessDestroy();
	SwapchainDestroy(&graphics_device->swapchain);
	
	vkDestroyPipelineCache(graphics_device->device, graphics_device->pipeline_process_cache, 0);
	vkDestroySurfaceKHR(graphics_device->instance, graphics_device->surface, 0);
	vmaDestroyAllocator(graphics_device->vma_allocator);
	vkDestroyDebugUtilsMessengerEXT(graphics_device->instance, graphics_device->debug_messenger, 0);
	vkDestroyDevice(graphics_device->device, 0);
}

internal CommandBuffer
BeginGraphicsPresent()
{
	GraphicsFrameData *current_frame = graphics_device->frames + graphics_device->current_frame_index;
	
	WaitForFence(current_frame->in_flight_fence);
	ResetFence(current_frame->in_flight_fence);
	
	SwapchainAcquireNextImage(&graphics_device->swapchain);
	
	CommandBuffer in_flight_cmd = FetchFreeCommandBuffer(&current_frame->command_pool);
	
	CmdBegin(&in_flight_cmd);
	
	return in_flight_cmd;
}

internal void
EndGraphicsPresent(CommandBuffer *in_flight_cmd)
{
	BindlessApplyUpdates();
	
	CmdTransitionImageLayout(in_flight_cmd,
							 GetCurrentSwapchainImage(&graphics_device->swapchain),
							 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	
	CmdEnd(in_flight_cmd);
	
	VkFence fence = graphics_device->frames[graphics_device->current_frame_index].in_flight_fence;
	
	VkSemaphoreSubmitInfo render_finished_semaphore = {0};
	render_finished_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore.semaphore = GetCurrentRenderFinishedSemaphore();
	render_finished_semaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	
	VkSemaphoreSubmitInfo image_available_semaphore = {0};
	image_available_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore.semaphore = GetCurrentImageAvailableSemaphore();
	image_available_semaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	
	VkCommandBufferSubmitInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = in_flight_cmd->handle;
	
	VkSubmitInfo2 submit_info = {0};
	{
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submit_info.flags = 0;
		
		submit_info.commandBufferInfoCount = 1;
		submit_info.pCommandBufferInfos = &buffer_info;
		
		submit_info.signalSemaphoreInfoCount = 1;
		submit_info.pSignalSemaphoreInfos = &render_finished_semaphore;
		
		submit_info.waitSemaphoreInfoCount = 1;
		submit_info.pWaitSemaphoreInfos = &image_available_semaphore;
	}
	
	VK_CHECK(vkQueueSubmit2(graphics_device->graphics_queue, 1, &submit_info, fence),
			 "Failed to submit in-flight draw command to buffer.");
	
	u32 image_index = graphics_device->swapchain.current_image_index;
	
	VkPresentInfoKHR present_info = {0};
	{
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &render_finished_semaphore.semaphore;
		
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &graphics_device->swapchain.handle;
		
		present_info.pImageIndices = &image_index;
		
		present_info.pResults = 0;
	}
	
	VkResult result = vkQueuePresentKHR(graphics_device->graphics_queue, &present_info);
	
	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		DebugLogCrash("TODO We need to rebuild the entire swapchain here.");
	}
	else if(result != VK_SUCCESS)
	{
		DebugLogCrash("Failed to present swapchain image.");
	}
	
	graphics_device->current_frame_index = (graphics_device->current_frame_index + 1) % FRAMES_IN_FLIGHT;
	
	vkQueueWaitIdle(graphics_device->graphics_queue);
	
	CommandPoolReset(&graphics_device->frames[graphics_device->current_frame_index].command_pool);
}

internal CommandBuffer
BeginGraphicsInstantSubmit()
{
	GraphicsFrameData *current_frame = graphics_device->frames + graphics_device->current_frame_index;
	
	WaitForFence(current_frame->instant_submit_fence);
	ResetFence(current_frame->instant_submit_fence);
	
	CommandBuffer instant_submit_cmd = FetchFreeCommandBuffer(&current_frame->command_pool);
	
	CmdBegin(&instant_submit_cmd);
	
	return instant_submit_cmd;
}

internal void
EndGraphicsInstantSubmit(CommandBuffer *instant_submit_cmd)
{
	GraphicsFrameData *current_frame = graphics_device->frames + graphics_device->current_frame_index;
	
	CmdEnd(instant_submit_cmd);
	
	VkCommandBufferSubmitInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = instant_submit_cmd->handle;
	
	VkFence fence = current_frame->instant_submit_fence;
	
	VkSubmitInfo2 submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;
	
	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;
	
	submit_info.signalSemaphoreInfoCount = 0;
	submit_info.waitSemaphoreInfoCount = 0;
	
	VK_CHECK(vkQueueSubmit2(graphics_device->graphics_queue, 1, &submit_info, fence),
			 "Failed to submit instant draw command to buffer.");
}
