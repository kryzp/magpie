
internal VkSurfaceFormatKHR SwapchainChooseSurfaceFormat(VkSurfaceFormatKHR *available_surface_formats,
							 u32 available_surface_format_count)
{
	VkSurfaceFormatKHR *format = available_surface_formats;

	for (i32 i = 0; i < available_surface_format_count; i++, format++) {
		if (format->format == VK_FORMAT_B8G8R8A8_UNORM &&
		    format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			DebugLog("Found *desired* swapchain swap surface format & colour space!");
			return *format;
		}
	}

	DebugLog("Could not find *desired* swapchain swap surface format & colour space, falling back...");

	return *available_surface_formats;
}

internal VkPresentModeKHR SwapchainChoosePresentMode(VkPresentModeKHR *available_present_modes,
						     u32 available_present_mode_count, b32 enable_vsync)
{
	if (!enable_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;

	VkPresentModeKHR *mode = available_present_modes;

	for (i32 i = 0; i < available_present_mode_count; i++, mode++) {
		if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

internal VkExtent2D SwapchainChooseExtent(const VkSurfaceCapabilitiesKHR *capabilities)
{
	if (capabilities->currentExtent.width != (u32)(-1) &&
	    capabilities->currentExtent.height != (u32)(-1))
		return capabilities->currentExtent;

	VkExtent2D actual_extent = {
		platform->window_width,
		platform->window_height
	};

	actual_extent.width = ClampValue(actual_extent.width,
					 capabilities->minImageExtent.width,
					 capabilities->maxImageExtent.width);

	actual_extent.height = ClampValue(actual_extent.height,
					  capabilities->minImageExtent.height,
					  capabilities->maxImageExtent.height);

	return actual_extent;
}

internal SwapchainSupportDetails QuerySwapchainSupport(MemoryArena *arena, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	SwapchainSupportDetails result = {0};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &result.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &result.surface_format_count, 0);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &result.present_mode_count, 0);

	if (result.surface_format_count >= 0) {
		result.surface_formats = MemoryArenaPushC(arena, result.surface_format_count, sizeof(VkSurfaceFormatKHR));
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
						     &result.surface_format_count,
						     result.surface_formats);
	}

	if (result.present_mode_count >= 0) {
		result.present_modes = MemoryArenaPushC(arena, result.present_mode_count, sizeof(VkPresentModeKHR));
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
							  &result.present_mode_count,
							  result.present_modes);
	}

	return result;
}

internal void SwapchainInit(Swapchain *swapchain, MemoryArena *arena)
{
	ScratchArena scratch = GetScratch(arena, 1);

	SwapchainSupportDetails details = QuerySwapchainSupport(scratch.arena,
								graphics_device->physical_device,
								graphics_device->surface);

	VkSurfaceFormatKHR surface_format = SwapchainChooseSurfaceFormat(details.surface_formats, details.surface_format_count);
	VkPresentModeKHR present_mode = SwapchainChoosePresentMode(details.present_modes, details.present_mode_count, true);
	VkExtent2D extent = SwapchainChooseExtent(&details.capabilities);

	swapchain->width = extent.width;
	swapchain->height = extent.height;
	swapchain->format = surface_format.format;

	u32 image_count = details.capabilities.minImageCount + 1;

	if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount)
		image_count = details.capabilities.maxImageCount;

	const VkImageUsageFlags swapchain_image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	
	VkSwapchainCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = graphics_device->surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = swapchain_image_usage;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = 0;
	create_info.preTransform = details.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK(vkCreateSwapchainKHR(graphics_device->device, &create_info, 0,
				      &swapchain->handle),
		 "Failed to create swapchain.");

	vkGetSwapchainImagesKHR(graphics_device->device, swapchain->handle, &image_count, 0);

	if (image_count <= 0)
		DebugLogCrash("Failed to find any images in swapchain.");

	swapchain->swapchain_image_count = image_count;

	swapchain->swapchain_images      = MemoryArenaPushC(arena, image_count, sizeof(Image));
	swapchain->swapchain_image_views = MemoryArenaPushC(arena, image_count, sizeof(ImageView));

	VkImage *vk_images = MemoryArenaPushC(scratch.arena, sizeof(VkImage), image_count);

	vkGetSwapchainImagesKHR(graphics_device->device, swapchain->handle, &image_count, vk_images);

	for (i32 i = 0; i < image_count; i++) {
		
		Image *image = swapchain->swapchain_images + i;

		image->image = vk_images[i];

		image->access_type = ImageAccessType_Undefined;
		
		image->width = swapchain->width;
		image->height = swapchain->height;
		image->depth = 1;

		image->is_swapchain = true;

		image->format = swapchain->format;
		image->type   = VK_IMAGE_VIEW_TYPE_2D;
		image->tiling = VK_IMAGE_TILING_OPTIMAL;

		image->usage = swapchain_image_usage;

		image->mipmap_count = 1;
		image->samples = VK_SAMPLE_COUNT_1_BIT;

		swapchain->swapchain_image_views[i] = ImageViewFromImage(image, ImageLayerCount(image), 0, 0);
	}

	ReleaseScratch(&scratch);

	DebugLog("Swapchain created.");
}

internal void SwapchainDestroy(Swapchain *swapchain)
{
	for (i32 i = 0; i < swapchain->swapchain_image_count; i++)
		ImageViewDestroy(swapchain->swapchain_image_views + i);

	vkDestroySwapchainKHR(graphics_device->device, swapchain->handle, NULL);
}

internal void SwapchainAcquireNextImage(Swapchain *swapchain)
{
	VkAcquireNextImageInfoKHR info = {0};
	info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	info.swapchain = swapchain->handle;
	info.timeout = UINT64_MAX;
	info.semaphore = GetCurrentImageAvailableSemaphore();
	info.fence = VK_NULL_HANDLE;
	info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(graphics_device->device, &info,
						 &swapchain->current_image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		DebugLogCrash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		DebugLogCrash("Failed to acquire next image in swapchain.");
}

internal Image *SwapchainCurrentImage(Swapchain *swapchain)
{
	return &swapchain->swapchain_images[swapchain->current_image_index];
}

internal ImageView *SwapchainCurrentImageView(Swapchain *swapchain)
{
	return &swapchain->swapchain_image_views[swapchain->current_image_index];
}
