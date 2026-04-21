
#define GFX_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)			\
	internal GFX_##mgp_name##Key										\
	GFX_Device##mgp_name##ListPush(GFX_Device##mgp_name##List *list, Arena *arena, const resource_name *resource, GFX_##mgp_name##Key key) \
	{																	\
		GFX_Device##mgp_name##Node *node = ArenaPushArray(arena, GFX_Device##mgp_name##Node, 1); \
		node->next = list->first;										\
		node->key = key;												\
		node->resource = *resource;										\
		list->first = node;												\
		return node->key;												\
	}																	\
	internal GFX_##mgp_name##Key										\
	GFX_Device##mgp_name##ListPushAuto(GFX_Device##mgp_name##List *list, Arena *arena, const resource_name *resource) \
	{																	\
		GFX_Device##mgp_name##Node *node = ArenaPushArray(arena, GFX_Device##mgp_name##Node, 1); \
		node->next = list->first;										\
		node->key.value = list->first ? list->first->key.value + 1 : 1; \
		node->resource = *resource;										\
		list->first = node;												\
		return node->key;												\
	}																	\
	internal resource_name *											\
	GFX_Device##mgp_name##ListGet(const GFX_Device##mgp_name##List *list, GFX_##mgp_name##Key key) \
	{																	\
		for (GFX_Device##mgp_name##Node *n = list->first; n; n = n->next) \
		{																\
			if (GFX_##mgp_name##KeyMatch(key, n->key))					\
				return &n->resource;									\
		}																\
		return NULL;													\
	}

#include "graphics_device_managed_resources.inc"

#undef GFX_DEVICE_MANAGED_RESOURCE

internal VkSurfaceFormatKHR
GFX_DeviceChooseSwapchainSurfaceFormat(u32 available_surface_format_count,
									   const VkSurfaceFormatKHR *available_surface_formats)
{
	for (u32 i = 0; i < available_surface_format_count; i++)
	{
		const VkSurfaceFormatKHR *format = &available_surface_formats[i];
		
		if (format->format == VK_FORMAT_B8G8R8A8_UNORM &&
		    format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			DebugLogF("Found desired swapchain swap surface format and colour space.");
			return *format;
		}
	}

	DebugLogF("Could not find desired swapchain swap surface format and colour space, falling back...");

	return available_surface_formats[0];
}


internal VkPresentModeKHR
GFX_DeviceChooseSwapchainPresentMode(u32 available_present_mode_count,
									 const VkPresentModeKHR *available_present_modes,
									 b32 enable_vsync)
{
	if (!enable_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for (u32 i = 0; i < available_present_mode_count; i++)
	{
		if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

internal VkExtent2D
GFX_DeviceChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR *capabilities)
{
	if (capabilities->currentExtent.width != ((u32)-1) &&
	    capabilities->currentExtent.height != ((u32)-1))
		return capabilities->currentExtent;

	u32 window_width;
	u32 window_height;

	osapi->GetWindowSize(&window_width, &window_height);
	
	VkExtent2D actual_extent = {
		(u32)window_width,
		(u32)window_height
	};

	actual_extent.width = ClampValue(actual_extent.width,
									 capabilities->minImageExtent.width,
									 capabilities->maxImageExtent.width);

	actual_extent.height = ClampValue(actual_extent.height,
									  capabilities->minImageExtent.height,
									  capabilities->maxImageExtent.height);

	return actual_extent;
}

internal u32
GFX_DeviceClampMipmapCount(u32 mipmaps, u32 w, u32 h, u32 d)
{
	u32 max_size = MaxValue(w, MaxValue(h, d));
	u32 max_mips = 1 + (u32)Log2F((f32)max_size);
	
	return MinValue(mipmaps, max_mips);
}

internal void
GFX_DeviceInit(GFX_Device *device, Arena *arena)
{
	device->permanent_arena = arena;
	device->frame_arena = ArenaInitArena(arena, arena->capacity * 0.5f, 8);

	device->current_frame_index = 0;

	device->context = GFX_ContextInit();

	GFX_DeviceCreateSyncResources(device);
	GFX_DeviceCreateBindless(device);
	GFX_DeviceCreateImGui(device);

	DebugLogF("Graphics Device Initialized.");
}

internal void
GFX_DeviceDestroy(GFX_Device *device)
{
	for (u32 i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
    {
        GFX_DevicePerFrameData *frame = &device->per_frame_data[i];
        GFX_DeviceWaitUntil(device, frame->completion_point);
        GFX_DeviceFlushFrameData(device, frame);
    }
	
	// Layouts, Pipelines and Views are special and cached
	// internally without any API to destroy them so we
	// have to destroy them here.
	for (GFX_DevicePipelineLayoutNode *node = device->layouts.first; node; node = node->next)
		vkDestroyPipelineLayout(device->context.device, node->resource, NULL);

	for (GFX_DevicePipelineNode *node = device->pipelines.first; node; node = node->next)
		vkDestroyPipeline(device->context.device, node->resource, NULL);

	for (GFX_DeviceTextureViewNode *node = device->views.first; node; node = node->next)
		vkDestroyImageView(device->context.device, node->resource.handle, NULL);
	
	GFX_DeviceDestroyImGui(device);
	GFX_DeviceDestroyBindless(device);
	GFX_DeviceDestroySyncResources(device);

	GFX_ContextDestroy(&device->context);

	DebugLogF("Graphics Device Destroyed.");
}

internal void
GFX_DeviceFlushFrameData(GFX_Device *device, GFX_DevicePerFrameData *frame_data)
{
	for (GFX_DestroyedImage *img = frame_data->destroyed_image_head; img; img = img->next)
		vmaDestroyImage(device->context.vma_allocator, img->image, img->allocation);

	for (GFX_DestroyedBuffer *b = frame_data->destroyed_buffer_head; b; b = b->next)
		vmaDestroyBuffer(device->context.vma_allocator, b->buffer, b->allocation);
	
	for (GFX_DestroyedSampler *s = frame_data->destroyed_sampler_head; s; s = s->next)
	{
		vkDestroySampler(device->context.device, s->sampler, NULL);
		GFX_BindlessFreeSampler(&device->bindless, s->bindless);
	}
	
	frame_data->destroyed_sampler_head = NULL;
	frame_data->destroyed_image_head = NULL;
	frame_data->destroyed_buffer_head = NULL;
}

// TODO: BeginFrame / EndFrame should be moved into the swapchain.

internal GFX_CmdBuffer
GFX_DeviceBeginFrame(GFX_Device *device, GFX_Swapchain *swapchain)
{
	GFX_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	GFX_DeviceFlushFrameData(device, frame_data);

	if (frame_data->completion_point.value > 0)
		GFX_DeviceWaitUntil(device, frame_data->completion_point);

	VkAcquireNextImageInfoKHR acquire_next_image_info = {0};
	acquire_next_image_info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquire_next_image_info.swapchain = swapchain->handle;
	acquire_next_image_info.timeout = UINT64_MAX;
	acquire_next_image_info.semaphore = frame_data->image_available_semaphore;
	acquire_next_image_info.fence = VK_NULL_HANDLE;
	acquire_next_image_info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(device->context.device, &acquire_next_image_info, &swapchain->current_texture_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		AssertTrue(false && "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		AssertTrue(false && "Failed to acquire next image in swapchain.");

	GFX_DeviceCmdPoolReset(device, &frame_data->command_pool);

	GFX_CmdBuffer cmd = GFX_DeviceFetchFreeBuffer(device, &frame_data->command_pool);
	GFX_CmdBegin(&cmd);

	return cmd;
}

internal void
GFX_DeviceEndFrame(GFX_Device *device, const GFX_Swapchain *swapchain, GFX_CmdBuffer *cmd)
{
	GFX_DeviceApplyBindlessUpdates(device);

	GFX_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	// TODO: Do we need to be waiting on VK_PIPELINE_STAGE_2_ALL_COMMANDS ????
	
	VkSemaphoreSubmitInfo image_available_semaphore = {0};
	image_available_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore.semaphore = frame_data->image_available_semaphore;
	image_available_semaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSemaphoreSubmitInfo render_finished_semaphore = {0};
	render_finished_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore.semaphore = frame_data->render_finished_semaphore;
	render_finished_semaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	frame_data->completion_point = GFX_DeviceSubmitEx(device, cmd,
													  1, &image_available_semaphore,
													  1, &render_finished_semaphore);
	
	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = NULL;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain->handle;
	present_info.pImageIndices = &swapchain->current_texture_index;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &frame_data->render_finished_semaphore;

	VkResult result = vkQueuePresentKHR(device->context.graphics_queue.handle, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		AssertTrue(false && "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		AssertTrue(false && "Failed to present swapchain image.");
	
	device->current_frame_index = (device->current_frame_index + 1) % GFX_FRAMES_IN_FLIGHT;

	ArenaClear(&device->frame_arena);
}

internal GFX_TimelinePoint
GFX_DeviceSubmit(GFX_Device *device, GFX_CmdBuffer *cmd)
{
	return GFX_DeviceSubmitEx(device, cmd, 0, NULL, 0, NULL);
}

internal GFX_TimelinePoint
GFX_DeviceSubmitEx(GFX_Device *device, GFX_CmdBuffer *cmd,
				   u32 wait_count, const VkSemaphoreSubmitInfo *waits,
				   u32 signal_count, const VkSemaphoreSubmitInfo *signals)
{
	GFX_CmdEnd(cmd);

	GFX_TimelinePoint timeline_point = GFX_SemaphoreSignal(&device->graphics_semaphore);

	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkSemaphoreSubmitInfo *all_signals = ArenaPushArray(scratch.arena, VkSemaphoreSubmitInfo, 1 + signal_count);

	VkSemaphoreSubmitInfo timeline_signal_info = {0};
	timeline_signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	timeline_signal_info.semaphore = timeline_point.semaphore;
	timeline_signal_info.value = timeline_point.value;
	timeline_signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	all_signals[0] = timeline_signal_info;

	for (u32 i = 0; i < signal_count; i++)
		all_signals[i + 1] = signals[i];

	VkCommandBufferSubmitInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = cmd->handle;

	VkSubmitInfo2 submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;

	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;

	submit_info.waitSemaphoreInfoCount = wait_count;
	submit_info.pWaitSemaphoreInfos = waits;

	submit_info.signalSemaphoreInfoCount = 1 + signal_count;
	submit_info.pSignalSemaphoreInfos = all_signals;

	GFX_VK_CHECK(vkQueueSubmit2(device->context.graphics_queue.handle, 1, &submit_info, VK_NULL_HANDLE),
				 "Failed to submit command to queue.");

	ScratchRelease(&scratch);

	return timeline_point;
}

internal GFX_CmdBuffer
GFX_DeviceSubmitImBegin(GFX_Device *device)
{
	vkQueueWaitIdle(device->context.graphics_queue.handle);

	GFX_CmdBuffer cmd = GFX_DeviceFetchFreeBuffer(device, &device->per_frame_data[device->current_frame_index].command_pool);
	GFX_CmdBegin(&cmd);

	return cmd;
}

internal void
GFX_DeviceSubmitImEnd(GFX_Device *device, GFX_CmdBuffer *cmd)
{
	GFX_DeviceWaitUntil(device, GFX_DeviceSubmit(device, cmd));
}

// TODO: We really shouldn't need to initialize
//       and de-initialize volk like this. Surely
//       we should load all fpointers into a table
//       which we store in program memory (possible
//       in volk) and just load that table back
//       in after reloading?
//       --> Also, do VMA's function pointers
//           break down here? Or is it okay?

internal void
GFX_DeviceHotLoad(GFX_Device *device)
{
	volkInitialize();
	volkLoadInstance(device->context.instance);
	volkLoadDevice(device->context.device);

	GFX_DeviceWaitIdle(device);
}

internal void
GFX_DeviceHotUnload(GFX_Device *device)
{
	volkFinalize();
}

internal void
GFX_DeviceQueryPoolDestroy(const GFX_Device *device, VkQueryPool pool)
{
	vkDestroyQueryPool(device->context.device, pool, NULL);
}

internal void
GFX_DeviceWaitIdle(const GFX_Device *device)
{
	vkDeviceWaitIdle(device->context.device);
}

internal void
GFX_DeviceWaitForFence(const GFX_Device *device, VkFence fence)
{
	vkWaitForFences(device->context.device, 1, &fence, VK_TRUE, UINT64_MAX);
}

internal void
GFX_DeviceResetFence(const GFX_Device *device, VkFence fence)
{
	vkResetFences(device->context.device, 1, &fence);
}

internal void
GFX_DeviceDestroyFence(const GFX_Device *device, VkFence fence)
{
	vkDestroyFence(device->context.device, fence, NULL);
}

internal GFX_Semaphore
GFX_DeviceSemaphoreCreate(const GFX_Device *device, u64 value)
{
	VkSemaphoreTypeCreateInfo timeline_type_create_info = {0};
	timeline_type_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	timeline_type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timeline_type_create_info.initialValue = value;

	VkSemaphoreCreateInfo timeline_semaphore_create_info = {0};
	timeline_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	timeline_semaphore_create_info.flags = 0;
	timeline_semaphore_create_info.pNext = &timeline_type_create_info;

	VkSemaphore vk_semaphore = VK_NULL_HANDLE;

	GFX_VK_CHECK(vkCreateSemaphore(device->context.device,
								   &timeline_semaphore_create_info, NULL,
								   &vk_semaphore),
				 "Failed to create timeline semaphore.");

	GFX_Semaphore semaphore = {0};
	semaphore.handle = vk_semaphore;
	semaphore.target = value;

	return semaphore;
}

internal void
GFX_DeviceSemaphoreDestroy(const GFX_Device *device, const GFX_Semaphore *semaphore)
{
	vkDestroySemaphore(device->context.device, semaphore->handle, NULL);
}

internal u64
GFX_DeviceSemaphoreValue(const GFX_Device *device, const GFX_Semaphore *semaphore)
{
	u64 result = 0;

	vkGetSemaphoreCounterValue(device->context.device,
							   semaphore->handle,
							   &result);

	return result;
}

internal void
GFX_DeviceWaitUntil(const GFX_Device *device, GFX_TimelinePoint point)
{
	if (point.value == 0)
		return;

	VkSemaphoreWaitInfo wait_info = {0};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores = &point.semaphore;
	wait_info.pValues = &point.value;

	GFX_VK_CHECK(vkWaitSemaphores(device->context.device,
								  &wait_info, UINT64_MAX),
				 "Failed to wait on timeline semaphore");
}

internal GFX_Swapchain
GFX_DeviceSwapchainCreate(const GFX_Device *device)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	const GFX_SwapchainSupportDetails *details = &device->context.swapchain_details;

	VkSurfaceFormatKHR surface_format = GFX_DeviceChooseSwapchainSurfaceFormat(details->surface_format_count, details->surface_formats);
	VkPresentModeKHR present_mode = GFX_DeviceChooseSwapchainPresentMode(details->present_mode_count, details->present_modes, false);
	VkExtent2D extent = GFX_DeviceChooseSwapchainExtent(&details->capabilities);

	GFX_Swapchain swapchain = {0};

	swapchain.width = extent.width;
	swapchain.height = extent.height;
	swapchain.format = surface_format.format;

	u32 texture_count = details->capabilities.minImageCount + 1;

	if (details->capabilities.maxImageCount > 0 && texture_count > details->capabilities.maxImageCount)
		texture_count = details->capabilities.maxImageCount;

	const VkImageUsageFlags swapchain_texture_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkSwapchainCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = device->context.surface;
	create_info.minImageCount = texture_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = swapchain_texture_usage;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = NULL;
	create_info.preTransform = details->capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	GFX_VK_CHECK(vkCreateSwapchainKHR(device->context.device,
									  &create_info, NULL,
									  &swapchain.handle),
				 "Failed to create swapchain.");

	vkGetSwapchainImagesKHR(device->context.device, swapchain.handle, &texture_count, NULL);

	if (texture_count <= 0)
		AssertTrue(false && "Failed to find any images in swapchain.");

	VkImage *vk_images = ArenaPushArray(scratch.arena, VkImage, texture_count);

	vkGetSwapchainImagesKHR(device->context.device, swapchain.handle, &texture_count, vk_images);

	swapchain.texture_count = texture_count;

	swapchain.textures = ArenaPushArray(device->permanent_arena, GFX_Texture,     texture_count);
	swapchain.views    = ArenaPushArray(device->permanent_arena, GFX_TextureView, texture_count);

	for (u32 i = 0; i < texture_count; i++)
	{
		GFX_Texture *texture = &swapchain.textures[i];
		texture->handle = vk_images[i];
		texture->width = swapchain.width;
		texture->height = swapchain.height;
		texture->depth = 1;
		texture->flags = GFX_TextureFlag_Swapchain;
		texture->format = swapchain.format;
		texture->type = VK_IMAGE_TYPE_2D;
		texture->tiling = VK_IMAGE_TILING_OPTIMAL;
		texture->usage = swapchain_texture_usage;
		texture->aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
		texture->layer_count = 1;
		texture->mipmap_count = 1;
		texture->sample_count = VK_SAMPLE_COUNT_1_BIT;

		swapchain.views[i].type = VK_IMAGE_VIEW_TYPE_2D;
		swapchain.views[i].range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
		swapchain.views[i].range.base_mip = 0;
		swapchain.views[i].range.mips = 1;
		swapchain.views[i].range.base_layer = 0;
		swapchain.views[i].range.layers = 1;

		VkImageViewCreateInfo view_create_info = {0};
		view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_create_info.image = vk_images[i];
		view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_create_info.format = swapchain.format;

		view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_create_info.subresourceRange.baseMipLevel = 0;
		view_create_info.subresourceRange.levelCount = 1;
		view_create_info.subresourceRange.baseArrayLayer = 0;
		view_create_info.subresourceRange.layerCount = 1;

		view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		GFX_VK_CHECK(vkCreateImageView(device->context.device,
									   &view_create_info, NULL,
									   &swapchain.views[i].handle),
					 "Failed to create texture image view.");
	}

	DebugLogF("Swapchain created.");

	ScratchRelease(&scratch);

	return swapchain;
}

internal void
GFX_DeviceSwapchainDestroy(const GFX_Device *device, const GFX_Swapchain *swapchain)
{
	for (u32 i = 0; i < swapchain->texture_count; i++)
		vkDestroyImageView(device->context.device, swapchain->views[i].handle, NULL);

	vkDestroySwapchainKHR(device->context.device, swapchain->handle, NULL);
}

internal GFX_CmdPool
GFX_DeviceCmdPoolCreate(const GFX_Device *device, u32 family_index)
{
	VkCommandPoolCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;

	GFX_CmdPool pool = {0};

	GFX_VK_CHECK(vkCreateCommandPool(device->context.device,
									 &create_info, NULL,
									 &pool.handle),
				 "Failed to create command pool.");

	VkCommandBufferAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = GFX_CMD_POOL_MAX_BUFFERS;
	alloc_info.commandPool = pool.handle;

	GFX_VK_CHECK(vkAllocateCommandBuffers(device->context.device,
										  &alloc_info,
										  pool.buffers),
				 "Failed to allocate command pool command buffers.");

	pool.used_count = 0;

	return pool;
}

internal void
GFX_DeviceCmdPoolDestroy(const GFX_Device *device, const GFX_CmdPool *pool)
{
	vkDestroyCommandPool(device->context.device, pool->handle, NULL);
}

internal void
GFX_DeviceCmdPoolReset(const GFX_Device *device, GFX_CmdPool *pool)
{
	pool->used_count = 0;
	vkResetCommandPool(device->context.device, pool->handle, 0);
}

internal GFX_CmdBuffer
GFX_DeviceFetchFreeBuffer(const GFX_Device *device, GFX_CmdPool *pool)
{
	AssertTrue(pool->used_count < GFX_CMD_POOL_MAX_BUFFERS);

	GFX_CmdBuffer cmd = {0};
	cmd.handle = pool->buffers[pool->used_count++];

	return cmd;
}

internal GFX_PipelineLayoutKey
GFX_DevicePipelineLayoutFetch(GFX_Device *device, GFX_ShaderKey program)
{
	u64 hashed_key_value = 0;
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, &program, sizeof(program));
	
	GFX_PipelineLayoutKey hashed_key = { hashed_key_value };
	
	if (GFX_DevicePipelineLayoutFromKey(device, hashed_key))
		return hashed_key;
	
	const GFX_ShaderProgram *gfx_program = GFX_DeviceShaderProgramFromKey(device, program);
	
	VkShaderStageFlags stage = GFX_ShaderProgramIsCompute(gfx_program)
		? VK_SHADER_STAGE_COMPUTE_BIT
		: VK_SHADER_STAGE_ALL_GRAPHICS;

	VkPushConstantRange push_constants = {0};
	push_constants.offset = 0;
	push_constants.size = gfx_program->push_constant_size;
	push_constants.stageFlags = stage;

	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = GFX_BindlessSetKind_COUNT;
	create_info.pSetLayouts = device->bindless.layouts;

	if (push_constants.size > 0)
	{
		create_info.pushConstantRangeCount = 1;
		create_info.pPushConstantRanges = &push_constants;
	}
	else
	{
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = NULL;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	GFX_VK_CHECK(vkCreatePipelineLayout(device->context.device,
										&create_info, NULL,
										&layout),
				 "Failed to create pipeline layout.");

	return GFX_DevicePipelineLayoutListPush(&device->layouts, device->permanent_arena, &layout, hashed_key);
}

internal void
GFX_DevicePipelineLayoutDestroy(GFX_Device *device, GFX_PipelineLayoutKey layout_key)
{
	VkPipelineLayout *layout = GFX_DevicePipelineLayoutListGet(&device->layouts, layout_key);
	if (layout)
		vkDestroyPipelineLayout(device->context.device, *layout, NULL);
}

internal GFX_PipelineKey
GFX_DeviceFetchGraphicsPipeline(GFX_Device *device, const GFX_GraphicsPipelineDef *def, GFX_PipelineLayoutKey layout_key)
{
	u64 hashed_key_value = 0;
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, def, sizeof(*def));
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, &layout_key, sizeof(layout_key));
	
	GFX_PipelineKey hashed_key = { hashed_key_value };
	
	if (GFX_DevicePipelineFromKey(device, hashed_key))
		return hashed_key;
	
	VkPipelineLayout layout = *GFX_DevicePipelineLayoutListGet(&device->layouts, layout_key);
	
	GFX_ShaderProgram *program = GFX_DeviceShaderProgramFromKey(device, def->program);

	AssertTrue(!GFX_ShaderProgramIsCompute(program));

	static const VkDynamicState graphics_pipeline_dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {0};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_state_create_info.pVertexBindingDescriptions = NULL;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_create_info.pVertexAttributeDescriptions = NULL;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {0};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = def->topology;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = NULL;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = NULL;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {0};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.f;
	rasterization_state_create_info.cullMode = def->cull_mode;
	rasterization_state_create_info.frontFace = def->front_face;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.f;
	rasterization_state_create_info.depthBiasClamp = 0.f;
	rasterization_state_create_info.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {0};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = def->min_sample_shading_enabled;
	multisample_state_create_info.minSampleShading = def->min_sample_shading;
	multisample_state_create_info.rasterizationSamples = def->samples;
	multisample_state_create_info.pSampleMask = NULL;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkPipelineColorBlendAttachmentState *blend_states = ArenaPushArray(scratch.arena, VkPipelineColorBlendAttachmentState, def->colour_attachment_count);

	for (u32 i = 0; i < def->colour_attachment_count; i++)
	{
		VkPipelineColorBlendAttachmentState *bs = &blend_states[i];

		MemZeroStruct(bs);

		bs->blendEnable         = def->blend_state.enabled;

		bs->srcColorBlendFactor = def->blend_state.colour.src;
		bs->dstColorBlendFactor = def->blend_state.colour.dst;
		bs->colorBlendOp        = def->blend_state.colour.op;

		bs->srcAlphaBlendFactor = def->blend_state.alpha.src;
		bs->dstAlphaBlendFactor = def->blend_state.alpha.dst;
		bs->alphaBlendOp        = def->blend_state.alpha.op;

		if (def->blend_state.write_mask[0]) bs->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (def->blend_state.write_mask[1]) bs->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (def->blend_state.write_mask[2]) bs->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (def->blend_state.write_mask[3]) bs->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {0};
	colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_create_info.logicOpEnable = def->blend_state.logic_op_enabled;
	colour_blend_state_create_info.logicOp = def->blend_state.logic_op;
	colour_blend_state_create_info.attachmentCount = def->colour_attachment_count;
	colour_blend_state_create_info.pAttachments = blend_states;
	colour_blend_state_create_info.blendConstants[0] = def->blend_state.constants[0];
	colour_blend_state_create_info.blendConstants[1] = def->blend_state.constants[1];
	colour_blend_state_create_info.blendConstants[2] = def->blend_state.constants[2];
	colour_blend_state_create_info.blendConstants[3] = def->blend_state.constants[3];

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {0};
	depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state_create_info.depthTestEnable       = def->depth_stencil_state.depth_test_enabled;
	depth_stencil_state_create_info.depthWriteEnable      = def->depth_stencil_state.depth_write_enabled;
	depth_stencil_state_create_info.depthCompareOp        = def->depth_stencil_state.depth_compare_op;
	depth_stencil_state_create_info.depthBoundsTestEnable = def->depth_stencil_state.depth_bounds_test_enabled;
	depth_stencil_state_create_info.minDepthBounds        = def->depth_stencil_state.depth_bounds_min;
	depth_stencil_state_create_info.maxDepthBounds        = def->depth_stencil_state.depth_bounds_max;
	depth_stencil_state_create_info.stencilTestEnable     = def->depth_stencil_state.stencil_test_enabled;
	depth_stencil_state_create_info.front.failOp          = def->depth_stencil_state.stencil_front.fail_op;
	depth_stencil_state_create_info.front.passOp          = def->depth_stencil_state.stencil_front.pass_op;
	depth_stencil_state_create_info.front.depthFailOp     = def->depth_stencil_state.stencil_front.depth_fail_op;
	depth_stencil_state_create_info.front.compareOp       = def->depth_stencil_state.stencil_front.compare_op;
	depth_stencil_state_create_info.front.writeMask       = def->depth_stencil_state.stencil_front.write_mask;
	depth_stencil_state_create_info.front.reference       = def->depth_stencil_state.stencil_front.reference;
	depth_stencil_state_create_info.back.failOp           = def->depth_stencil_state.stencil_back.fail_op;
	depth_stencil_state_create_info.back.passOp           = def->depth_stencil_state.stencil_back.pass_op;
	depth_stencil_state_create_info.back.depthFailOp      = def->depth_stencil_state.stencil_back.depth_fail_op;
	depth_stencil_state_create_info.back.compareOp        = def->depth_stencil_state.stencil_back.compare_op;
	depth_stencil_state_create_info.back.writeMask        = def->depth_stencil_state.stencil_back.write_mask;
	depth_stencil_state_create_info.back.reference        = def->depth_stencil_state.stencil_back.reference;

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {0};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = ArraySize(graphics_pipeline_dynamic_states);
	dynamic_state_create_info.pDynamicStates = graphics_pipeline_dynamic_states;

	VkFormat depth_stencil_format = def->has_depth_attachment
		? device->context.depth_format
		: VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {0};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.viewMask = def->multi_view_mask;
	pipeline_rendering_create_info.colorAttachmentCount = def->colour_attachment_count;
	pipeline_rendering_create_info.pColorAttachmentFormats = def->colour_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat = depth_stencil_format;
	pipeline_rendering_create_info.stencilAttachmentFormat = depth_stencil_format;

	VkShaderModuleCreateInfo module_infos[GFX_MAX_SHADER_STAGES] = {0};
	VkPipelineShaderStageCreateInfo shader_stages[GFX_MAX_SHADER_STAGES] = {0};

	for (u32 i = 0; i < program->stage_count; i++)
	{
		module_infos[i].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_infos[i].codeSize = program->stages[i].bytecode.size;
		module_infos[i].pCode = (u32 *)program->stages[i].bytecode.bytes;
		module_infos[i].flags = 0;

		shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[i].stage = (VkShaderStageFlagBits)program->stages[i].flags;
		shader_stages[i].module = VK_NULL_HANDLE;
		shader_stages[i].pName = "main";
		shader_stages[i].pNext = &module_infos[i];
	}

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {0};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = program->stage_count;
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

	GFX_VK_CHECK(vkCreateGraphicsPipelines(device->context.device,
										   device->context.pipeline_process_cache,
										   1, &graphics_pipeline_create_info,
										   NULL, &pipeline),
				 "Failed to create graphics pipeline.");

	ScratchRelease(&scratch);

	return GFX_DevicePipelineListPush(&device->pipelines, device->permanent_arena, &pipeline, hashed_key);
}

internal GFX_PipelineKey
GFX_DeviceFetchComputePipeline(GFX_Device *device, const GFX_ComputePipelineDef *def, GFX_PipelineLayoutKey layout_key)
{
	u64 hashed_key_value = 0;
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, def, sizeof(*def));
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, &layout_key, sizeof(layout_key));
	
	GFX_PipelineKey hashed_key = { hashed_key_value };
	
	if (GFX_DevicePipelineFromKey(device, hashed_key))
		return hashed_key;
	
	VkPipelineLayout layout = *GFX_DevicePipelineLayoutListGet(&device->layouts, layout_key);
	GFX_ShaderProgram *program = GFX_DeviceShaderProgramFromKey(device, def->program);

	AssertTrue(GFX_ShaderProgramIsCompute(program));

	VkShaderModuleCreateInfo module_info = {0};
	module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_info.codeSize = program->stages[0].bytecode.size;
	module_info.pCode = (u32 *)program->stages[0].bytecode.bytes;
	module_info.flags = 0;

	VkPipelineShaderStageCreateInfo shader_stage = {0};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage = (VkShaderStageFlagBits)program->stages[0].flags;
	shader_stage.module = VK_NULL_HANDLE;
	shader_stage.pName = "main";
	shader_stage.pNext = &module_info;

	VkComputePipelineCreateInfo compute_pipeline_create_info = {0};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.stage = shader_stage;

	VkPipeline pipeline = VK_NULL_HANDLE;

	GFX_VK_CHECK(vkCreateComputePipelines(device->context.device,
										   device->context.pipeline_process_cache,
										  1, &compute_pipeline_create_info,
										  NULL, &pipeline),
				 "Failed to create compute pipeline.");

	return GFX_DevicePipelineListPush(&device->pipelines, device->permanent_arena, &pipeline, hashed_key);
}

internal VkPipelineLayout
GFX_DevicePipelineLayoutFromKey(const GFX_Device *device, GFX_PipelineLayoutKey key)
{
	VkPipelineLayout *layout = GFX_DevicePipelineLayoutListGet(&device->layouts, key);
	return layout ? *layout : VK_NULL_HANDLE;
}

internal VkPipeline
GFX_DevicePipelineFromKey(const GFX_Device *device, GFX_PipelineKey key)
{
	VkPipeline *pipeline = GFX_DevicePipelineListGet(&device->pipelines, key);
	return pipeline ? *pipeline : VK_NULL_HANDLE;
}

internal GFX_TextureKey
GFX_DeviceTextureAlloc(GFX_Device *device, const GFX_TextureAllocInfo *alloc_info)
{
	GFX_Texture texture = {0};

	texture.width  = alloc_info->width;
	texture.height = alloc_info->height;
	texture.depth  = alloc_info->depth;

	texture.format = alloc_info->format;
	texture.type   = alloc_info->type;
	texture.tiling = alloc_info->tiling;

	texture.mipmap_count = GFX_DeviceClampMipmapCount(alloc_info->mipmaps, alloc_info->width, alloc_info->height, alloc_info->depth);
	texture.layer_count  = alloc_info->layers;
	
	texture.sample_count = alloc_info->samples;

	texture.flags = 0;

	if (alloc_info->flags & GFX_TextureAllocFlag_Transient)
	{
		texture.flags |= GFX_TextureFlag_Transient;
		texture.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
	else
	{
		texture.usage =
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	b32 is_depth   = (alloc_info->format == device->context.depth_format);
	b32 is_cubemap = (alloc_info->flags & GFX_TextureAllocFlag_Cubemap) != 0;
	b32 is_storage = (alloc_info->flags & GFX_TextureAllocFlag_Storage) != 0;

	if (is_depth)
	{
		texture.flags |= GFX_TextureFlag_Depth;
		texture.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		
		texture.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		texture.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		texture.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageCreateFlags vk_create_flags = 0;
	VmaAllocationCreateFlags vma_alloc_flags = 0;
	
	if (is_cubemap)
	{
		texture.flags |= GFX_TextureFlag_Cubemap;
		
		vk_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	
	if (is_storage)
	{
		texture.flags |= GFX_TextureFlag_Storage;
		texture.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

		vma_alloc_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	}

	/*
	texture.aspect_count = 0;

	for (VkImageAspectFlags b = 1; b <= texture.aspect_flags; b <<= 1)
	{
		if (texture.aspect_flags & b)
			texture.aspect_count++;
	}
	*/

	VkImageCreateInfo create_info = {0};
	create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType     = alloc_info->type;
	create_info.extent.width  = texture.width;
	create_info.extent.height = texture.height;
	create_info.extent.depth  = texture.depth;
	create_info.mipLevels     = texture.mipmap_count;
	create_info.arrayLayers   = texture.layer_count;
	create_info.format        = texture.format;
	create_info.tiling        = texture.tiling;
	create_info.usage         = texture.usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples       = texture.sample_count;
	create_info.flags         = vk_create_flags;

	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.priority = 1.f;
	vma_alloc_info.flags = vma_alloc_flags;

	GFX_VK_CHECK(vmaCreateImage(device->context.vma_allocator,
								&create_info,
								&vma_alloc_info,
								&texture.handle,
								&texture.allocation,
								&texture.allocation_info),
				 "Failed to allocate texture.");

	return GFX_DeviceTextureListPushAuto(&device->textures, device->permanent_arena, &texture);
}

internal GFX_TextureKey
GFX_DeviceTextureAlloc2D(GFX_Device *device, u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	GFX_TextureAllocInfo alloc_info = {0};
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = GFX_TextureAllocFlag_None;

	return GFX_DeviceTextureAlloc(device, &alloc_info);
}

internal GFX_TextureKey
GFX_DeviceTextureAlloc2DRW(GFX_Device *device, u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	GFX_TextureAllocInfo alloc_info = {0};
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = GFX_TextureAllocFlag_Storage;

	return GFX_DeviceTextureAlloc(device, &alloc_info);
}

internal GFX_TextureKey
GFX_DeviceTextureAllocDepth2D(GFX_Device *device, u32 width, u32 height, u32 mipmaps)
{
	return GFX_DeviceTextureAlloc2D(device, width, height, device->context.depth_format, mipmaps);
}

internal GFX_TextureKey
GFX_DeviceTextureAllocDepth2DRW(GFX_Device *device, u32 width, u32 height, u32 mipmaps)
{
	return GFX_DeviceTextureAlloc2DRW(device, width, height, device->context.depth_format, mipmaps);
}

internal GFX_TextureKey
GFX_DeviceTextureAllocCubemap(GFX_Device *device, u32 resolution, VkFormat format, u32 mipmaps)
{
	GFX_TextureAllocInfo alloc_info = {0};
	alloc_info.width = resolution;
	alloc_info.height = resolution;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 6;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = GFX_TextureAllocFlag_Cubemap;

	return GFX_DeviceTextureAlloc(device, &alloc_info);
}

internal GFX_TextureKey
GFX_DeviceTextureAllocCubemapDepth(GFX_Device *device, u32 resolution, u32 mipmaps)
{
	return GFX_DeviceTextureAllocCubemap(device, resolution, device->context.depth_format, mipmaps);
}

internal void
GFX_DeviceTextureDestroy(GFX_Device *device, GFX_TextureKey texture_key)
{
	GFX_Texture *texture = GFX_DeviceTextureListGet(&device->textures, texture_key);
	AssertTrue(texture);

	GFX_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	GFX_DestroyedImage *node = ArenaPushArray(&device->frame_arena, GFX_DestroyedImage, 1);
	node->image = texture->handle;
	node->allocation = texture->allocation;
	node->next = frame_data->destroyed_image_head;
	frame_data->destroyed_image_head = node;
}

internal GFX_Texture *
GFX_DeviceTextureFromKey(const GFX_Device *device, GFX_TextureKey key)
{
	return GFX_DeviceTextureListGet(&device->textures, key);
}

internal GFX_TextureViewKey
GFX_DeviceTextureViewFetch(GFX_Device *device, const GFX_TextureViewCreateInfo *info)
{
	GFX_TextureViewKey hashed_key = { HashBytesGeneric(info, sizeof(*info)) };
	
	if (GFX_DeviceTextureViewFromKey(device, hashed_key))
		return hashed_key;
	
	GFX_Texture *gfx_texture = GFX_DeviceTextureFromKey(device, info->texture);
	
	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = gfx_texture->handle;
	view_create_info.viewType = info->type;
	view_create_info.format = gfx_texture->format;

	view_create_info.subresourceRange.aspectMask     = info->range.aspects;
	view_create_info.subresourceRange.baseMipLevel   = info->range.base_mip;
	view_create_info.subresourceRange.levelCount     = info->range.mips;
	view_create_info.subresourceRange.baseArrayLayer = info->range.base_layer;
	view_create_info.subresourceRange.layerCount     = info->range.layers;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	GFX_TextureView view = {0};
	view.type = info->type;
	view.range = info->range;

	GFX_VK_CHECK(vkCreateImageView(device->context.device,
								   &view_create_info, NULL,
								   &view.handle),
				 "Failed to create texture image view.");

	b32 is_storage = (gfx_texture->flags & GFX_TextureFlag_Storage) != 0;

	view.bindless = GFX_BindlessRegisterView(&device->bindless, view.handle, is_storage);
	
	return GFX_DeviceTextureViewListPush(&device->views, device->permanent_arena, &view, hashed_key);
}

internal GFX_TextureViewKey
GFX_DeviceTextureViewAuto(GFX_Device *device, GFX_TextureKey texture)
{
	GFX_Texture *gfx_texture = GFX_DeviceTextureFromKey(device, texture);

	AssertTrue(gfx_texture);

	GFX_SubresourceRange range = {0};
	range.aspects = gfx_texture->aspect_flags;
	range.base_mip = 0;
	range.mips = gfx_texture->mipmap_count;
	range.base_layer = 0;
	range.layers = gfx_texture->layer_count;

	VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
	
	if (gfx_texture->flags & GFX_TextureFlag_Cubemap)
		view_type = VK_IMAGE_VIEW_TYPE_CUBE;

	GFX_TextureViewCreateInfo info = {0};
	info.texture = texture;
	info.type = view_type;
	info.range = range;

	return GFX_DeviceTextureViewFetch(device, &info);
}

internal GFX_TextureView *
GFX_DeviceTextureViewFromKey(const GFX_Device *device, GFX_TextureViewKey key)
{
	return GFX_DeviceTextureViewListGet(&device->views, key);
}

internal GFX_BufferKey
GFX_DeviceBufferAlloc(GFX_Device *device, const GFX_BufferAllocInfo *alloc_info)
{
	GFX_Buffer buffer = {0};
	buffer.usage = alloc_info->usage;
	buffer.size = alloc_info->size;
	buffer.allocation_flags = alloc_info->flags;
	buffer.allocator = device->context.vma_allocator;

	b32 is_storage = (buffer.usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0;

	if (is_storage)
		buffer.usage |= VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferCreateInfo buffer_create_info = {0};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = buffer.size;
	buffer_create_info.usage = buffer.usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = NULL;

	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = alloc_info->flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	GFX_VK_CHECK(vmaCreateBuffer(device->context.vma_allocator,
								 &buffer_create_info,
								 &vma_alloc_info,
								 &buffer.handle,
								 &buffer.allocation,
								 &buffer.allocation_info),
				 "Failed to allocate buffer.");

	if (is_storage)
	{
		VkBufferDeviceAddressInfo address_info = {0};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer.handle;

		buffer.device_address = vkGetBufferDeviceAddress(device->context.device, &address_info);
	}

	return GFX_DeviceBufferListPushAuto(&device->buffers, device->permanent_arena, &buffer);
}

internal GFX_BufferKey
GFX_DeviceStageAlloc(GFX_Device *device, u64 size)
{
	GFX_BufferAllocInfo alloc_info = {0};
	alloc_info.usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
	alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	alloc_info.size = size;

	return GFX_DeviceBufferAlloc(device, &alloc_info);
}

internal void
GFX_DeviceBufferDestroy(GFX_Device *device, GFX_BufferKey buffer_key)
{
	GFX_Buffer *buffer = GFX_DeviceBufferListGet(&device->buffers, buffer_key);
	AssertTrue(buffer);

	GFX_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	GFX_DestroyedBuffer *node = ArenaPushArray(&device->frame_arena, GFX_DestroyedBuffer, 1);
	node->buffer = buffer->handle;
	node->allocation = buffer->allocation;
	node->next = frame_data->destroyed_buffer_head;
	frame_data->destroyed_buffer_head = node;
}

internal GFX_Buffer *
GFX_DeviceBufferFromKey(const GFX_Device *device, GFX_BufferKey key)
{
	return GFX_DeviceBufferListGet(&device->buffers, key);
}

internal GFX_SamplerKey
GFX_DeviceSamplerCreate(GFX_Device *device, const GFX_SamplerCreateInfo *info)
{
	VkPhysicalDeviceProperties properties = device->context.physical_device_properties.properties;

	VkSamplerCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.minFilter = info->filter;
	create_info.magFilter = info->filter;
	create_info.addressModeU = info->wrap_x;
	create_info.addressModeV = info->wrap_y;
	create_info.addressModeW = info->wrap_z;
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	create_info.borderColor = info->border_colour;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.f;
	create_info.minLod = 0.f;
	create_info.maxLod = VK_LOD_CLAMP_NONE;

	GFX_Sampler sampler = {0};
	
	GFX_VK_CHECK(vkCreateSampler(device->context.device,
								 &create_info, NULL,
								 &sampler.handle),
				 "Failed to create texture sampler.");

	sampler.filter = info->filter;
	sampler.wrap_x = info->wrap_x;
	sampler.wrap_y = info->wrap_y;
	sampler.wrap_z = info->wrap_z;
	sampler.border_colour = info->border_colour;
	sampler.bindless = GFX_BindlessRegisterSampler(&device->bindless, sampler.handle);

	return GFX_DeviceSamplerListPushAuto(&device->samplers, device->permanent_arena, &sampler);
}

internal GFX_SamplerKey
GFX_DeviceSamplerCreateF(GFX_Device *device, VkFilter filter)
{
	GFX_SamplerCreateInfo create_info = {0};
	create_info.filter = filter;
	create_info.wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.border_colour = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	return GFX_DeviceSamplerCreate(device, &create_info);
}

internal void
GFX_DeviceSamplerDestroy(GFX_Device *device, GFX_SamplerKey sampler_key)
{
	GFX_Sampler *sampler = GFX_DeviceSamplerListGet(&device->samplers, sampler_key);
	
	AssertTrue(sampler);

	GFX_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	GFX_DestroyedSampler *node = ArenaPushArray(&device->frame_arena, GFX_DestroyedSampler, 1);
	node->sampler = sampler->handle;
	node->bindless = sampler->bindless;
	node->next = frame_data->destroyed_sampler_head;
	frame_data->destroyed_sampler_head = node;
}

internal GFX_Sampler *
GFX_DeviceSamplerFromKey(const GFX_Device *device, GFX_SamplerKey key)
{
	return GFX_DeviceSamplerListGet(&device->samplers, key);
}

internal GFX_ShaderStage
GFX_DeviceShaderStageCreate(Arena *arena, const GFX_ShaderBytecode *bytecode)
{
	SpvReflectShaderModule reflect_module = {0};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(bytecode->size, bytecode->bytes, &reflect_module);

	if (reflect_result != SPV_REFLECT_RESULT_SUCCESS)
	{
		DebugLogF("Failed to reflect SPIR-V module: %d\n", reflect_result);
		AssertTrue(false);
	}
	
	ScratchArena scratch = ScratchBegin(&arena, 1);

	GFX_ShaderStage stage = {0};

	if (reflect_module.entry_point_count >= 1)
	{
		stage.flags = (VkShaderStageFlagBits)reflect_module.entry_points[0].shader_stage;

		u32 push_constant_count = 0;
		reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, NULL);

		if (reflect_result == SPV_REFLECT_RESULT_SUCCESS && push_constant_count > 0)
		{
			SpvReflectBlockVariable **pcs = ArenaPushArray(scratch.arena, SpvReflectBlockVariable *, push_constant_count);
			
			spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, pcs);

			for (u32 i = 0; i < push_constant_count; i++)
			{
				SpvReflectBlockVariable *pc = pcs[i];

				u32 alignment = 4;

				for (u32 j = 0; j < pc->member_count; j++)
					alignment = MaxValue(alignment, pc->members[j].size);

				u32 padded = MemAlignUp(pc->size, alignment);

				stage.push_constant_size = MaxValue(stage.push_constant_size, padded);
			}
		}

		stage.bytecode.size = bytecode->size;
		stage.bytecode.bytes = ArenaPush(arena, bytecode->size, 1);
		
		MemCopy(stage.bytecode.bytes, bytecode->bytes, bytecode->size);
	} else {
		DebugLogF("No entry points found in SPIR-V.");
		AssertTrue(false);
	}

	spvReflectDestroyShaderModule(&reflect_module);

	ScratchRelease(&scratch);

	return stage;
}

internal GFX_ShaderKey
GFX_DeviceShaderProgramCreate(GFX_Device *device, u32 stage_count, const GFX_ShaderBytecode *stages)
{
	GFX_ShaderProgram program = {0};
	program.stage_count = stage_count;
	program.push_constant_size = 0;

	for (u32 i = 0; i < stage_count; i++)
	{
		program.stages[i] = GFX_DeviceShaderStageCreate(device->permanent_arena, &stages[i]);
		program.push_constant_size = MaxValue(program.push_constant_size, program.stages[i].push_constant_size);
	}

	static u32 shader_cookie = 0;
	program.cookie = ++shader_cookie;

	return GFX_DeviceShaderListPushAuto(&device->shaders, device->permanent_arena, &program);
}

internal void
GFX_DeviceShaderProgramDestroy(GFX_Device *device, GFX_ShaderKey program_key)
{
	// TODO
}

internal GFX_ShaderProgram *
GFX_DeviceShaderProgramFromKey(const GFX_Device *device, GFX_ShaderKey key)
{
	return GFX_DeviceShaderListGet(&device->shaders, key);
}

internal void
GFX_DeviceCreateSyncResources(GFX_Device *device)
{
	device->graphics_semaphore = GFX_DeviceSemaphoreCreate(device, 0);

	u32 family_index = device->context.graphics_queue.family_index;

	for (u32 i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
	{
		device->per_frame_data[i].completion_point.value = 0;
		device->per_frame_data[i].completion_point.semaphore = device->graphics_semaphore.handle;

		device->per_frame_data[i].command_pool = GFX_DeviceCmdPoolCreate(device, family_index);

		VkSemaphoreCreateInfo binary_semaphore_create_info = {0};
		binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		binary_semaphore_create_info.flags = 0;
		binary_semaphore_create_info.pNext = NULL;

		GFX_VK_CHECK(vkCreateSemaphore(device->context.device,
									   &binary_semaphore_create_info, NULL,
									   &device->per_frame_data[i].image_available_semaphore),
					 "Failed to create image available semaphore.");

		GFX_VK_CHECK(vkCreateSemaphore(device->context.device,
									   &binary_semaphore_create_info, NULL,
									   &device->per_frame_data[i].render_finished_semaphore),
					 "Failed to create render finished semaphore.");

		device->per_frame_data[i].destroyed_sampler_head = NULL;
		device->per_frame_data[i].destroyed_image_head = NULL;
		//device->per_frame_data[i].destroyed_view_head = NULL;
		device->per_frame_data[i].destroyed_buffer_head = NULL;
	}

	DebugLogF("Created frame sync objects.");
}

internal void
GFX_DeviceDestroySyncResources(GFX_Device *device)
{
	for (u32 i = 0; i < GFX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device->context.device, device->per_frame_data[i].image_available_semaphore, NULL);
		vkDestroySemaphore(device->context.device, device->per_frame_data[i].render_finished_semaphore, NULL);
		GFX_DeviceCmdPoolDestroy(device, &device->per_frame_data[i].command_pool);
		GFX_DeviceFlushFrameData(device, &device->per_frame_data[i]);
	}

	GFX_DeviceSemaphoreDestroy(device, &device->graphics_semaphore);
}

internal void
GFX_DeviceCreateBindless(GFX_Device *device)
{
	VkDescriptorPoolSize pool_sizes[GFX_BindlessSetKind_COUNT] = {0};

	for (u32 i = 0; i < GFX_BindlessSetKind_COUNT; i++)
	{
		pool_sizes[i].type = GFX_BindlessGetVkType((GFX_BindlessSetKind)i);
		pool_sizes[i].descriptorCount = GFX_BINDLESS_MAX_RESOURCES;
	}

	VkDescriptorPoolCreateInfo pool_create_info = {0};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = ArraySize(pool_sizes) * GFX_BINDLESS_MAX_RESOURCES;
	pool_create_info.poolSizeCount = ArraySize(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;

	GFX_VK_CHECK(vkCreateDescriptorPool(device->context.device,
										&pool_create_info, NULL,
										&device->bindless.pool),
				 "Failed to create bindless descriptor pool.");

	VkDescriptorBindingFlags bindless_flags =
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
		VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {0};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = 1;
	binding_flags.pBindingFlags = &bindless_flags;

	for (u32 i = 0; i < GFX_BindlessSetKind_COUNT; i++)
	{
		VkDescriptorSetLayoutBinding binding = {0};
		binding.descriptorType = GFX_BindlessGetVkType((GFX_BindlessSetKind)i);
		binding.descriptorCount = GFX_BINDLESS_MAX_RESOURCES;
		binding.binding = 0;
		binding.stageFlags = VK_SHADER_STAGE_ALL;
		binding.pImmutableSamplers = NULL;

		VkDescriptorSetLayoutCreateInfo layout_create_info = {0};
		layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_create_info.bindingCount = 1;
		layout_create_info.pBindings = &binding;
		layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		layout_create_info.pNext = &binding_flags;

		GFX_VK_CHECK(vkCreateDescriptorSetLayout(device->context.device,
												 &layout_create_info, NULL,
												 &device->bindless.layouts[i]),
					 "Failed to create bindless descriptor layout.");
	}

	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = device->bindless.pool;
	alloc_info.descriptorSetCount = GFX_BindlessSetKind_COUNT;
	alloc_info.pSetLayouts = device->bindless.layouts;

	GFX_VK_CHECK(vkAllocateDescriptorSets(device->context.device,
										  &alloc_info,
										  device->bindless.sets),
				 "Failed to allocate bindless descriptor set.");

	DebugLogF("Bindless resources created.");
}

internal void
GFX_DeviceDestroyBindless(GFX_Device *device)
{
	for (u32 i = 0; i < GFX_BindlessSetKind_COUNT; i++)
		vkDestroyDescriptorSetLayout(device->context.device, device->bindless.layouts[i], NULL);

	vkDestroyDescriptorPool(device->context.device, device->bindless.pool, NULL);
}

internal void
GFX_DeviceApplyBindlessUpdates(GFX_Device *device)
{
	if (device->bindless.update_count == 0)
		return;

	ScratchArena scratch = ScratchBegin(NULL, 0);

	u32 count = device->bindless.update_count;

	VkWriteDescriptorSet *writes = ArenaPushArray(scratch.arena, VkWriteDescriptorSet, count);
	VkDescriptorImageInfo *infos = ArenaPushArray(scratch.arena, VkDescriptorImageInfo, count);

	for (u32 i = 0; i < count; i++)
	{
		GFX_BindlessUpdate *update = &device->bindless.updates[i];

		infos[i].sampler = update->sampler;
		infos[i].imageView = update->view;
		infos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		writes[i] = (VkWriteDescriptorSet){0};
		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].descriptorCount = 1;
		writes[i].dstArrayElement = update->slot;
		writes[i].descriptorType = GFX_BindlessGetVkType(update->kind);
		writes[i].dstSet = device->bindless.sets[update->kind];
		writes[i].dstBinding = 0;
		writes[i].pImageInfo = &infos[i];
	}

	vkUpdateDescriptorSets(device->context.device,
						   count, writes,
						   0, NULL);

	device->bindless.update_count = 0;

	ScratchRelease(&scratch);
}

internal void
GFX_DeviceCreateImGui(GFX_Device *device)
{
	/*
	const u32 max_sets = 1000;

	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                max_sets },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_sets },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, max_sets },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       max_sets }
	};

	VkDescriptorPoolCreateInfo pool_info = {0};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = max_sets;
	pool_info.poolSizeCount = ArraySize(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	GFX_VK_CHECK(vkCreateDescriptorPool(device->context.device,
										&pool_info, NULL,
										&device->imgui_pool),
				 "Failed to create ImGui descriptor pool.");

	VkFormat swapchain_image_format = VK_FORMAT_R32G32B32A32_SFLOAT;

	ImGui_ImplVulkan_InitInfo init_info = {0};
	init_info.Instance = device->context.instance;
	init_info.PhysicalDevice = device->context.physical_device;
	init_info.Device = device->context.device;
	init_info.QueueFamily = device->context.graphics_queue.family_index;
	init_info.Queue = device->context.graphics_queue.handle;
	init_info.PipelineCache = device->pipeline_cache;
	init_info.DescriptorPool = device->imgui_pool;
	init_info.Allocator = NULL;
	init_info.MinImageCount = GFX_FRAMES_IN_FLIGHT;
	init_info.ImageCount = GFX_FRAMES_IN_FLIGHT;
	init_info.CheckVkResultFn = NULL;
	init_info.UseDynamicRendering = true;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_image_format;

	ImGui_ImplVulkan_Init(&init_info);
	*/
}

internal void
GFX_DeviceDestroyImGui(GFX_Device *device)
{
	/*
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(device->context.device, device->imgui_pool, NULL);
	*/
}

internal void
GFX_DeviceImGuiNewFrame(const GFX_Device *device)
{
	/*
	ImGui_ImplVulkan_NewFrame();
	*/
}

internal void
GFX_DeviceImGuiRecord(const GFX_Device *device, const GFX_CmdBuffer *cmd)
{
	/*
	ImDrawData *draw_data = ImGui_GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, cmd->handle);
	*/
}
