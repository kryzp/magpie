
#define G_DEVICE_MANAGED_RESOURCE(mgp_name, resource_name)				\
	static G_##mgp_name##Key G_Device##mgp_name##ListPush(G_Device##mgp_name##List *list, Arena *arena, const resource_name *resource, G_##mgp_name##Key key) \
	{																	\
		G_Device##mgp_name##Node *node = ArenaPushArray(arena, G_Device##mgp_name##Node, 1); \
		node->next = list->first;										\
		node->key = key;												\
		node->resource = *resource;										\
		list->first = node;												\
		return node->key;												\
	}																	\
	static G_##mgp_name##Key G_Device##mgp_name##ListPushAuto(G_Device##mgp_name##List *list, Arena *arena, const resource_name *resource) \
	{																	\
		G_Device##mgp_name##Node *node = ArenaPushArray(arena, G_Device##mgp_name##Node, 1); \
		node->next = list->first;										\
		node->key.value = list->first ? list->first->key.value + 1 : 1; \
		node->resource = *resource;										\
		list->first = node;												\
		return node->key;												\
	}																	\
	static resource_name *G_Device##mgp_name##ListGet(const G_Device##mgp_name##List *list, G_##mgp_name##Key key) \
	{																	\
		for (G_Device##mgp_name##Node *n = list->first; n; n = n->next) \
		{																\
			if (G_##mgp_name##KeyMatch(key, n->key))					\
				return &n->resource;									\
		}																\
		return NULL;													\
	}

#include "graphics_device_managed_resources.inc"

#undef G_DEVICE_MANAGED_RESOURCE

static VkSurfaceFormatKHR G_DeviceChooseSwapchainSurfaceFormat(LOG_Channel channel,
									 u32 available_surface_format_count,
									 const VkSurfaceFormatKHR *available_surface_formats)
{
	for (u32 i = 0; i < available_surface_format_count; i++)
	{
		const VkSurfaceFormatKHR *format = &available_surface_formats[i];
		
		if (format->format == VK_FORMAT_B8G8R8A8_UNORM &&
		    format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			DebugLogD(channel, "Found desired swapchain swap surface format and colour space.");
			return *format;
		}
	}

	DebugLogB(channel, "Could not find desired swapchain swap surface format and colour space.");

	return available_surface_formats[0];
}


static VkPresentModeKHR G_DeviceChooseSwapchainPresentMode(u32 available_present_mode_count,
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

static VkExtent2D G_DeviceChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR *capabilities)
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

static u32 G_DeviceClampMipmapCount(u32 mipmaps, u32 w, u32 h, u32 d)
{
	u32 max_size = MaxValue(w, MaxValue(h, d));
	u32 max_mips = 1 + (u32)Log2F((f32)max_size);
	
	return MinValue(mipmaps, max_mips);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL G_DeviceVulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
							VkDebugUtilsMessageTypeFlagsEXT message_type,
							const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
							void *ctx)
{
	G_Device *device = ctx;

	LOG_Channel ch = device->log_channel;

	switch (message_type)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			ch = device->log_channel_general;
			break;
			
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			ch = device->log_channel_validation;
			break;
			
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			ch = device->log_channel_performance;
			break;
	}

	switch (message_severity)
	{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			DebugLogT(ch, "%s", callback_data->pMessage);
			break;
		
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			DebugLogD(ch, "%s", callback_data->pMessage);
			break;
		
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			DebugLogW(ch, "%s", callback_data->pMessage);
			break;
		
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			DebugLogB(ch, "%s", callback_data->pMessage);
			break;
	}
	
	return VK_FALSE;
}

static void G_DeviceInit(G_Device *device, Arena *arena, LOG_Channel log_channel)
{
	device->permanent_arena = arena;

	device->log_channel = log_channel;

	device->log_channel_general     = osapi->LogChannelOpenFrom(log_channel, String8Lit("GENERAL"));
	device->log_channel_validation  = osapi->LogChannelOpenFrom(log_channel, String8Lit("VALIDATION"));
	device->log_channel_performance = osapi->LogChannelOpenFrom(log_channel, String8Lit("PERFORMANCE"));
	
	for (u32 i = 0; i < ArraySize(device->per_frame_data); i++)
		device->per_frame_data[i].arena = ArenaAlloc(Megabytes(128));

	device->current_frame_index = 0;

	device->context = G_ContextInit(device->log_channel, G_DeviceVulkanDebugCallback, device);

	G_DeviceCreateSyncResources(device);
	G_DeviceCreateBindless(device);
	G_DeviceCreateImGui(device);

	DebugLogI(device->log_channel, "Initialized.");
}

static void G_DeviceDestroy(G_Device *device)
{
	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
    {
        G_DevicePerFrameData *frame = &device->per_frame_data[i];
        G_DeviceWaitUntil(device, frame->completion_point);
        G_DeviceFlushFrameData(device, frame);

		ArenaRelease(&frame->arena);
    }
	
	// Layouts, Pipelines and Views are special and cached
	// internally without any API to destroy them so we
	// have to destroy them here.
	for (G_DevicePipelineLayoutNode *node = device->layouts.first; node; node = node->next)
		vkDestroyPipelineLayout(device->context.device, node->resource, NULL);

	for (G_DevicePipelineNode *node = device->pipelines.first; node; node = node->next)
		vkDestroyPipeline(device->context.device, node->resource, NULL);

	for (G_DeviceTextureViewNode *node = device->views.first; node; node = node->next)
		vkDestroyImageView(device->context.device, node->resource.vk_handle, NULL);
	
	G_DeviceDestroyImGui(device);
	G_DeviceDestroyBindless(device);
	G_DeviceDestroySyncResources(device);

	G_ContextDestroy(&device->context);

	DebugLogI(device->log_channel, "Destroyed.");
}

static void G_DeviceFlushFrameData(G_Device *device, G_DevicePerFrameData *frame_data)
{
	for (G_DestroyedImage *img = frame_data->destroyed_image_head; img; img = img->next)
		vmaDestroyImage(device->context.vma_allocator, img->image, img->allocation);

	for (G_DestroyedBuffer *b = frame_data->destroyed_buffer_head; b; b = b->next)
		vmaDestroyBuffer(device->context.vma_allocator, b->buffer, b->allocation);
	
	for (G_DestroyedSampler *s = frame_data->destroyed_sampler_head; s; s = s->next)
	{
		vkDestroySampler(device->context.device, s->sampler, NULL);
		G_BindlessFreeSampler(&device->bindless, s->bindless);
	}
	
	frame_data->destroyed_sampler_head = NULL;
	frame_data->destroyed_image_head = NULL;
	frame_data->destroyed_buffer_head = NULL;

	ArenaReset(&frame_data->arena);
}

// TODO: BeginFrame / EndFrame should be moved into the swapchain.

static G_CmdBuffer G_DeviceBeginFrame(G_Device *device, G_Swapchain *swapchain)
{
	G_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	G_DeviceFlushFrameData(device, frame_data);

	if (frame_data->completion_point.value > 0)
		G_DeviceWaitUntil(device, frame_data->completion_point);

	VkAcquireNextImageInfoKHR acquire_next_image_info = {0};
	acquire_next_image_info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquire_next_image_info.swapchain = swapchain->vk_handle;
	acquire_next_image_info.timeout = UINT64_MAX;
	acquire_next_image_info.semaphore = frame_data->image_available_semaphore;
	acquire_next_image_info.fence = VK_NULL_HANDLE;
	acquire_next_image_info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(device->context.device, &acquire_next_image_info, &swapchain->current_texture_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		DebugLogB(device->log_channel, "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		DebugLogB(device->log_channel, "Failed to acquire next image in swapchain.");

	G_DeviceCmdPoolPurge(device, &frame_data->command_pool, frame_data->completion_point.value);

	G_CmdBuffer cmd = G_DeviceCmdPoolAcquire(device, &frame_data->command_pool);
	G_CmdBegin(&cmd);

	return cmd;
}

static void G_DeviceEndFrame(G_Device *device, const G_Swapchain *swapchain, const G_CmdBuffer *cmd)
{
	G_DeviceApplyBindlessUpdates(device);

	G_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	// TODO: Do we need to be waiting on VK_PIPELINE_STAGE_2_ALL_COMMANDS ????
	
	VkSemaphoreSubmitInfo image_available_semaphore = {0};
	image_available_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore.semaphore = frame_data->image_available_semaphore;
	image_available_semaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSemaphoreSubmitInfo render_finished_semaphore = {0};
	render_finished_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore.semaphore = frame_data->render_finished_semaphore;
	render_finished_semaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	frame_data->completion_point = G_DeviceSubmitEx(device, cmd,
													1, &image_available_semaphore,
													1, &render_finished_semaphore);
	
	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = NULL;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain->vk_handle;
	present_info.pImageIndices = &swapchain->current_texture_index;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &frame_data->render_finished_semaphore;

	VkResult result = vkQueuePresentKHR(device->context.graphics_queue.vk_handle, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		DebugLogB(device->log_channel, "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		DebugLogB(device->log_channel, "Failed to present swapchain image.");

	device->current_frame_index = (device->current_frame_index + 1) % G_FRAMES_IN_FLIGHT;
	
	G_DeviceCmdPoolRelease(device, &frame_data->command_pool, cmd, frame_data->completion_point.value);
}

static G_TimelinePoint G_DeviceSubmit(G_Device *device, const G_CmdBuffer *cmd)
{
	return G_DeviceSubmitEx(device, cmd, 0, NULL, 0, NULL);
}

static G_TimelinePoint G_DeviceSubmitEx(G_Device *device, const G_CmdBuffer *cmd,
				 u32 wait_count, const VkSemaphoreSubmitInfo *waits,
				 u32 signal_count, const VkSemaphoreSubmitInfo *signals)
{
	G_CmdEnd(cmd);

	G_TimelinePoint timeline_point = G_SemaphoreSignal(&device->graphics_semaphore);

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
	buffer_info.deviceMask = 1;
	buffer_info.commandBuffer = cmd->vk_handle;

	VkSubmitInfo2 submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;

	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;

	submit_info.waitSemaphoreInfoCount = wait_count;
	submit_info.pWaitSemaphoreInfos = waits;

	submit_info.signalSemaphoreInfoCount = 1 + signal_count;
	submit_info.pSignalSemaphoreInfos = all_signals;

	G_VK_CHECK(vkQueueSubmit2(device->context.graphics_queue.vk_handle, 1, &submit_info, VK_NULL_HANDLE),
			   "Failed to submit command to queue.");

	ScratchRelease(&scratch);

	return timeline_point;
}

static G_CmdBuffer G_DeviceSubmitImBegin(G_Device *device)
{
	vkQueueWaitIdle(device->context.graphics_queue.vk_handle);

	G_CmdBuffer cmd = G_DeviceCmdPoolAcquire(device, &device->per_frame_data[device->current_frame_index].command_pool);
	G_CmdBegin(&cmd);

	return cmd;
}

static void G_DeviceSubmitImEnd(G_Device *device, const G_CmdBuffer *cmd)
{
	G_DeviceWaitUntil(device, G_DeviceSubmit(device, cmd));
}

// TODO: We really shouldn't need to initialize
//       and de-initialize volk like this. Surely
//       we should load all fpointers into a table
//       which we store in program memory (possible
//       in volk) and just load that table back
//       in after reloading?
//       --> Also, do VMA's function pointers
//           break down here? Or is it okay?

static void G_DeviceHotLoad(G_Device *device)
{
	volkInitialize();
	volkLoadInstance(device->context.instance);
	volkLoadDevice(device->context.device);

	G_DeviceWaitIdle(device);
}

static void G_DeviceHotUnload(G_Device *device)
{
	volkFinalize();
}

static void G_DeviceQueryPoolDestroy(const G_Device *device, VkQueryPool pool)
{
	vkDestroyQueryPool(device->context.device, pool, NULL);
}

static void G_DeviceWaitIdle(const G_Device *device)
{
	vkDeviceWaitIdle(device->context.device);
}

static void G_DeviceWaitForFence(const G_Device *device, VkFence fence)
{
	vkWaitForFences(device->context.device, 1, &fence, VK_TRUE, UINT64_MAX);
}

static void G_DeviceResetFence(const G_Device *device, VkFence fence)
{
	vkResetFences(device->context.device, 1, &fence);
}

static void G_DeviceDestroyFence(const G_Device *device, VkFence fence)
{
	vkDestroyFence(device->context.device, fence, NULL);
}

static G_Semaphore G_DeviceSemaphoreCreate(const G_Device *device, u64 value)
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

	G_VK_CHECK(vkCreateSemaphore(device->context.device,
								 &timeline_semaphore_create_info, NULL,
								 &vk_semaphore),
			   "Failed to create timeline semaphore.");

	G_Semaphore semaphore = {0};
	semaphore.vk_handle = vk_semaphore;
	semaphore.target = value;

	return semaphore;
}

static void G_DeviceSemaphoreDestroy(const G_Device *device, const G_Semaphore *semaphore)
{
	vkDestroySemaphore(device->context.device, semaphore->vk_handle, NULL);
}

static u64 G_DeviceSemaphoreValue(const G_Device *device, const G_Semaphore *semaphore)
{
	u64 result = 0;

	vkGetSemaphoreCounterValue(device->context.device,
							   semaphore->vk_handle,
							   &result);

	return result;
}

static void G_DeviceWaitUntil(const G_Device *device, G_TimelinePoint point)
{
	if (point.value == 0)
		return;

	VkSemaphoreWaitInfo wait_info = {0};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores = &point.semaphore;
	wait_info.pValues = &point.value;

	G_VK_CHECK(vkWaitSemaphores(device->context.device,
								&wait_info, UINT64_MAX),
			   "Failed to wait on timeline semaphore");
}

static G_Swapchain G_DeviceSwapchainCreate(G_Device *device)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	const G_SwapchainSupportDetails *details = &device->context.swapchain_details;

	VkSurfaceFormatKHR surface_format = G_DeviceChooseSwapchainSurfaceFormat(device->log_channel, details->surface_format_count, details->surface_formats);
	VkPresentModeKHR present_mode = G_DeviceChooseSwapchainPresentMode(details->present_mode_count, details->present_modes, false); // TODO: add option to enable VSYNC
	VkExtent2D extent = G_DeviceChooseSwapchainExtent(&details->capabilities);

	G_Swapchain swapchain = {0};

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

	G_VK_CHECK(vkCreateSwapchainKHR(device->context.device,
									&create_info, NULL,
									&swapchain.vk_handle),
			   "Failed to create swapchain.");

	vkGetSwapchainImagesKHR(device->context.device, swapchain.vk_handle, &texture_count, NULL);

	DebugLogAssert(device->log_channel, texture_count > 0, "Failed to find any images in swapchain.");

	VkImage *vk_images = ArenaPushArray(scratch.arena, VkImage, texture_count);

	vkGetSwapchainImagesKHR(device->context.device, swapchain.vk_handle, &texture_count, vk_images);

	swapchain.texture_count = texture_count;

	swapchain.textures = ArenaPushArray(device->permanent_arena, G_Texture,     texture_count);
	swapchain.views    = ArenaPushArray(device->permanent_arena, G_TextureView, texture_count);

	for (u32 i = 0; i < texture_count; i++)
	{
		G_Texture texture = {0};
		texture.vk_handle = vk_images[i];
		texture.width = swapchain.width;
		texture.height = swapchain.height;
		texture.depth = 1;
		texture.flags = G_TextureFlag_Swapchain;
		texture.format = swapchain.format;
		texture.type = VK_IMAGE_TYPE_2D;
		texture.tiling = VK_IMAGE_TILING_OPTIMAL;
		texture.usage = swapchain_texture_usage;
		texture.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
		texture.layer_count = 1;
		texture.mipmap_count = 1;
		texture.sample_count = VK_SAMPLE_COUNT_1_BIT;

		swapchain.textures[i] = G_DeviceTextureListPushAuto(&device->textures, device->permanent_arena, &texture);

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

		G_VK_CHECK(vkCreateImageView(device->context.device,
									 &view_create_info, NULL,
									 &swapchain.views[i].vk_handle),
				   "Failed to create texture image view.");
	}

	DebugLogD(device->log_channel, "Swapchain created.");

	ScratchRelease(&scratch);

	return swapchain;
}

static void G_DeviceSwapchainDestroy(const G_Device *device, const G_Swapchain *swapchain)
{
	for (u32 i = 0; i < swapchain->texture_count; i++)
		vkDestroyImageView(device->context.device, swapchain->views[i].vk_handle, NULL);

	vkDestroySwapchainKHR(device->context.device, swapchain->vk_handle, NULL);
}

static G_CmdPool G_DeviceCmdPoolCreate(const G_Device *device, u32 family_index)
{
	VkCommandPoolCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;

	G_CmdPool pool = {0};

	G_VK_CHECK(vkCreateCommandPool(device->context.device,
								   &create_info, NULL,
								   &pool.vk_handle),
			   "Failed to create command pool.");

	pool.acquire_count = 0;

	pool.release_front = 0;
	pool.release_count = 0;

	return pool;
}

static void G_DeviceCmdPoolDestroy(const G_Device *device, const G_CmdPool *pool)
{
	vkDestroyCommandPool(device->context.device, pool->vk_handle, NULL);
}

static G_CmdBuffer G_DeviceCmdPoolAcquire(G_Device *device, G_CmdPool *pool)
{
	VkCommandBuffer cb = VK_NULL_HANDLE;
	
	if (pool->acquire_count > 0)
	{
        cb = pool->acquire_stack[--pool->acquire_count];
		
        vkResetCommandBuffer(cb, 0);

		//DebugLogD(device->log_channel, "Reused");
	}
	else
	{
		VkCommandBufferAllocateInfo alloc_info = {0};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;
		alloc_info.commandPool = pool->vk_handle;
		
		G_VK_CHECK(vkAllocateCommandBuffers(device->context.device,
											&alloc_info,
											&cb),
				   "Failed to allocate command pool command buffers.");
		
		//DebugLogD(device->log_channel, "Allocation");
	}

	return G_CmdInit(cb, device);
}

static void G_DeviceCmdPoolRelease(const G_Device *device, G_CmdPool *pool, const G_CmdBuffer *cmd, u64 fence_value)
{
	DebugLogAssert(device->log_channel,
				   pool->release_count < ArraySize(pool->release_queue),
				   "Command pool release queue is full.");
	
    u32 slot = (pool->release_front + pool->release_count) % ArraySize(pool->release_queue);

	G_CmdPoolReleasedBuffer *released = &pool->release_queue[slot];
	
	released->vk_handle = cmd->vk_handle;
    released->fence_value = fence_value;

	pool->release_count++;
}

static void G_DeviceCmdPoolPurge(const G_Device *device, G_CmdPool *pool, u64 fence_value)
{
	while (pool->release_count > 0)
	{
        G_CmdPoolReleasedBuffer *released = &pool->release_queue[pool->release_front];

        if (released->fence_value > fence_value)
            break;

        DebugLogAssert(device->log_channel,
                       pool->acquire_count < ArraySize(pool->acquire_stack),
                       "Command pool acquire stack is full.");

        pool->acquire_stack[pool->acquire_count++] = released->vk_handle;
        pool->release_front = (pool->release_front + 1) % ArraySize(pool->release_queue);
        pool->release_count--;
	}
}

static G_PipelineLayoutKey G_DevicePipelineLayoutFetch(G_Device *device, G_ShaderKey program)
{
	u64 hashed_key_value = 0;
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, &program, sizeof(program));
	
	G_PipelineLayoutKey hashed_key = { hashed_key_value };
	
	if (G_DevicePipelineLayoutFromKey(device, hashed_key))
		return hashed_key;
	
	const G_ShaderProgram *gfx_program = G_DeviceShaderProgramFromKey(device, program);
	
	VkShaderStageFlags stage = G_ShaderProgramIsCompute(gfx_program)
		? VK_SHADER_STAGE_COMPUTE_BIT
		: VK_SHADER_STAGE_ALL_GRAPHICS;

	VkPushConstantRange push_constants = {0};
	push_constants.offset = 0;
	push_constants.size = gfx_program->push_constant_size;
	push_constants.stageFlags = stage;

	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &device->bindless.layout;

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

	G_VK_CHECK(vkCreatePipelineLayout(device->context.device,
									  &create_info, NULL,
									  &layout),
			   "Failed to create pipeline layout.");

	return G_DevicePipelineLayoutListPush(&device->layouts, device->permanent_arena, &layout, hashed_key);
}

static VkPipelineLayout G_DevicePipelineLayoutFromKey(const G_Device *device, G_PipelineLayoutKey key)
{
	VkPipelineLayout *layout = G_DevicePipelineLayoutListGet(&device->layouts, key);
	return layout ? *layout : VK_NULL_HANDLE;
}

static G_PipelineSt G_DeviceFetchGraphicsPipeline(G_Device *device, const G_GraphicsPipelineDef *def)
{
	G_PipelineLayoutKey layout_key = G_DevicePipelineLayoutFetch(device, def->program);
	
	u64 hashed_key_value = 0;
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, def, sizeof(*def));
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, &layout_key, sizeof(layout_key));
	
	G_PipelineKey hashed_key = { hashed_key_value };
	
	if (G_DevicePipelineFromKey(device, hashed_key))
	{
		G_PipelineSt st = {0};
		st.pipeline = hashed_key;
		st.layout = layout_key;
		st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
		
		return st;
	}
	
	VkPipelineLayout layout = *G_DevicePipelineLayoutListGet(&device->layouts, layout_key);
	
	G_ShaderProgram *program = G_DeviceShaderProgramFromKey(device, def->program);

	AssertTrue(!G_ShaderProgramIsCompute(program));

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
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE; // todo: add option to turn this on
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

	VkShaderModuleCreateInfo module_infos[G_MAX_SHADER_STAGES] = {0};
	VkPipelineShaderStageCreateInfo shader_stages[G_MAX_SHADER_STAGES] = {0};

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

	G_VK_CHECK(vkCreateGraphicsPipelines(device->context.device,
										 device->context.pipeline_process_cache,
										 1, &graphics_pipeline_create_info,
										 NULL, &pipeline),
			   "Failed to create graphics pipeline.");

	G_PipelineSt st = {0};
	st.pipeline = G_DevicePipelineListPush(&device->pipelines, device->permanent_arena, &pipeline, hashed_key);
	st.layout = layout_key;
	st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	ScratchRelease(&scratch);
	
	return st;
}

static G_PipelineSt G_DeviceFetchComputePipeline(G_Device *device, const G_ComputePipelineDef *def)
{
	G_PipelineLayoutKey layout_key = G_DevicePipelineLayoutFetch(device, def->program);
	
	u64 hashed_key_value = 0;
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, def, sizeof(*def));
	hashed_key_value = HashBytesGenericCombine(hashed_key_value, &layout_key, sizeof(layout_key));
	
	G_PipelineKey hashed_key = { hashed_key_value };
	
	if (G_DevicePipelineFromKey(device, hashed_key))
	{
		G_PipelineSt st = {0};
		st.pipeline = hashed_key;
		st.layout = layout_key;
		st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
		
		return st;
	}
	
	VkPipelineLayout layout = *G_DevicePipelineLayoutListGet(&device->layouts, layout_key);
	
	G_ShaderProgram *program = G_DeviceShaderProgramFromKey(device, def->program);

	AssertTrue(G_ShaderProgramIsCompute(program));

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

	G_VK_CHECK(vkCreateComputePipelines(device->context.device,
										device->context.pipeline_process_cache,
										1, &compute_pipeline_create_info,
										NULL, &pipeline),
			   "Failed to create compute pipeline.");

	G_PipelineSt st = {0};
	st.pipeline = G_DevicePipelineListPush(&device->pipelines, device->permanent_arena, &pipeline, hashed_key);
	st.layout = layout_key;
	st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
	
	return st;
}

static VkPipeline G_DevicePipelineFromKey(const G_Device *device, G_PipelineKey key)
{
	VkPipeline *pipeline = G_DevicePipelineListGet(&device->pipelines, key);
	return pipeline ? *pipeline : VK_NULL_HANDLE;
}

static G_TextureKey G_DeviceTextureAlloc(G_Device *device, const G_TextureAllocInfo *alloc_info)
{
	G_Texture texture = {0};

	texture.width  = alloc_info->width;
	texture.height = alloc_info->height;
	texture.depth  = alloc_info->depth;

	texture.format = alloc_info->format;
	texture.type   = alloc_info->type;
	texture.tiling = alloc_info->tiling;

	texture.mipmap_count = G_DeviceClampMipmapCount(alloc_info->mipmaps, alloc_info->width, alloc_info->height, alloc_info->depth);
	texture.layer_count  = alloc_info->layers;
	
	texture.sample_count = alloc_info->samples;

	texture.flags = 0;

	if (alloc_info->flags & G_TextureAllocFlag_Transient)
	{
		texture.flags |= G_TextureFlag_Transient;
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
	b32 is_cubemap = (alloc_info->flags & G_TextureAllocFlag_Cubemap) != 0;
	b32 is_storage = (alloc_info->flags & G_TextureAllocFlag_Storage) != 0;

	if (is_depth)
	{
		texture.flags |= G_TextureFlag_Depth;
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
		texture.flags |= G_TextureFlag_Cubemap;
		
		vk_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}
	
	if (is_storage)
	{
		texture.flags |= G_TextureFlag_Storage;
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

	G_VK_CHECK(vmaCreateImage(device->context.vma_allocator,
							  &create_info,
							  &vma_alloc_info,
							  &texture.vk_handle,
							  &texture.allocation,
							  &texture.allocation_info),
			   "Failed to allocate texture.");

	return G_DeviceTextureListPushAuto(&device->textures, device->permanent_arena, &texture);
}

static G_TextureKey G_DeviceTextureAlloc2D(G_Device *device, u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	G_TextureAllocInfo alloc_info = {0};
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = G_TextureAllocFlag_None;

	return G_DeviceTextureAlloc(device, &alloc_info);
}

static G_TextureKey G_DeviceTextureAlloc2DRW(G_Device *device, u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	G_TextureAllocInfo alloc_info = {0};
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = G_TextureAllocFlag_Storage;

	return G_DeviceTextureAlloc(device, &alloc_info);
}

static G_TextureKey G_DeviceTextureAllocDepth2D(G_Device *device, u32 width, u32 height, u32 mipmaps)
{
	return G_DeviceTextureAlloc2D(device, width, height, device->context.depth_format, mipmaps);
}

static G_TextureKey G_DeviceTextureAllocDepth2DRW(G_Device *device, u32 width, u32 height, u32 mipmaps)
{
	return G_DeviceTextureAlloc2DRW(device, width, height, device->context.depth_format, mipmaps);
}

static G_TextureKey G_DeviceTextureAllocCubemap(G_Device *device, u32 resolution, VkFormat format, u32 mipmaps)
{
	G_TextureAllocInfo alloc_info = {0};
	alloc_info.width = resolution;
	alloc_info.height = resolution;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 6;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = G_TextureAllocFlag_Cubemap;

	return G_DeviceTextureAlloc(device, &alloc_info);
}

static G_TextureKey G_DeviceTextureAllocCubemapDepth(G_Device *device, u32 resolution, u32 mipmaps)
{
	return G_DeviceTextureAllocCubemap(device, resolution, device->context.depth_format, mipmaps);
}

static void G_DeviceTextureDestroy(G_Device *device, G_TextureKey texture_key)
{
	G_Texture *texture = G_DeviceTextureListGet(&device->textures, texture_key);
	DebugLogAssert(device->log_channel, texture, "Invalid texture with key %llu when destroying.", texture_key.value);

	G_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	G_DestroyedImage *node = ArenaPushArray(&frame_data->arena, G_DestroyedImage, 1);
	node->image = texture->vk_handle;
	node->allocation = texture->allocation;
	node->next = frame_data->destroyed_image_head;
	frame_data->destroyed_image_head = node;
}

static G_Texture *G_DeviceTextureFromKey(const G_Device *device, G_TextureKey key)
{
	return G_DeviceTextureListGet(&device->textures, key);
}

static G_TextureViewKey G_DeviceTextureViewFetch(G_Device *device, const G_TextureViewCreateInfo *info)
{
	G_TextureViewKey hashed_key = { HashBytesGeneric(info, sizeof(*info)) };
	
	if (G_DeviceTextureViewFromKey(device, hashed_key))
		return hashed_key;
	
	G_Texture *gfx_texture = G_DeviceTextureFromKey(device, info->texture);
	DebugLogAssert(device->log_channel, gfx_texture, "Invalid texture with key %llu when creating view.", info->texture.value);
	
	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = gfx_texture->vk_handle;
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

	G_TextureView view = {0};
	view.type = info->type;
	view.range = info->range;

	G_VK_CHECK(vkCreateImageView(device->context.device,
								 &view_create_info, NULL,
								 &view.vk_handle),
			   "Failed to create texture image view.");

	b32 is_cubemap = (gfx_texture->flags & G_TextureFlag_Cubemap) != 0;
	b32 is_storage = (gfx_texture->flags & G_TextureFlag_Storage) != 0;

	view.bindless = G_BindlessRegisterView(&device->bindless, view.vk_handle, is_cubemap, is_storage);
	
	return G_DeviceTextureViewListPush(&device->views, device->permanent_arena, &view, hashed_key);
}

static G_TextureViewKey G_DeviceTextureViewAuto(G_Device *device, G_TextureKey texture)
{
	G_Texture *gfx_texture = G_DeviceTextureFromKey(device, texture);
	DebugLogAssert(device->log_channel, gfx_texture, "Invalid texture with key %llu when creating view (auto).", texture.value);

	G_SubresourceRange range = {0};
	range.aspects = gfx_texture->aspect_flags;
	range.base_mip = 0;
	range.mips = gfx_texture->mipmap_count;
	range.base_layer = 0;
	range.layers = gfx_texture->layer_count;

	VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
	
	if (gfx_texture->flags & G_TextureFlag_Cubemap)
		view_type = VK_IMAGE_VIEW_TYPE_CUBE;

	G_TextureViewCreateInfo info = {0};
	info.texture = texture;
	info.type = view_type;
	info.range = range;

	return G_DeviceTextureViewFetch(device, &info);
}

static G_TextureView *G_DeviceTextureViewFromKey(const G_Device *device, G_TextureViewKey key)
{
	return G_DeviceTextureViewListGet(&device->views, key);
}

static G_BindlessIndex G_DeviceTextureViewBindless(const G_Device *device, G_TextureViewKey key)
{
	G_TextureView *gfx_view = G_DeviceTextureViewFromKey(device, key);
	DebugLogAssert(device->log_channel, gfx_view, "Invalid texture view with key %llu when getting bindless info.", key.value);
	
	return G_BindlessIndexOf(gfx_view->bindless);
}

static G_BufferKey G_DeviceBufferAlloc(G_Device *device, const G_BufferAllocInfo *alloc_info)
{
	G_Buffer buffer = {0};
	buffer.usage = alloc_info->usage;
	buffer.size = alloc_info->size;
	buffer.allocation_flags = alloc_info->flags;

	// just implicitly make all storage buffers have device address
	if ((buffer.usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0)
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
	vma_alloc_info.flags = alloc_info->flags | VMA_ALLOCATION_CREATE_MAPPED_BIT; // just map every buffer it's easier and the performance loss is negligible i rekon
	
	G_VK_CHECK(vmaCreateBuffer(device->context.vma_allocator,
							   &buffer_create_info,
							   &vma_alloc_info,
							   &buffer.vk_handle,
							   &buffer.allocation,
							   &buffer.allocation_info),
			   "Failed to allocate buffer.");

	if ((buffer.usage & VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT) != 0)
	{
		VkBufferDeviceAddressInfo address_info = {0};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer.vk_handle;

		buffer.device_address = vkGetBufferDeviceAddress(device->context.device, &address_info);
	}

	return G_DeviceBufferListPushAuto(&device->buffers, device->permanent_arena, &buffer);
}

static G_BufferKey G_DeviceStageAlloc(G_Device *device, u64 size)
{
	G_BufferAllocInfo alloc_info = {0};
	alloc_info.usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
	alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	alloc_info.size = size;

	return G_DeviceBufferAlloc(device, &alloc_info);
}

static void G_DeviceBufferDestroy(G_Device *device, G_BufferKey buffer_key)
{
	G_Buffer *buffer = G_DeviceBufferListGet(&device->buffers, buffer_key);
	DebugLogAssert(device->log_channel, buffer, "Invalid buffer with key %llu when destroying.", buffer_key.value);

	G_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	G_DestroyedBuffer *node = ArenaPushArray(&frame_data->arena, G_DestroyedBuffer, 1);
	node->buffer = buffer->vk_handle;
	node->allocation = buffer->allocation;
	node->next = frame_data->destroyed_buffer_head;
	frame_data->destroyed_buffer_head = node;
}

static G_Buffer *G_DeviceBufferFromKey(const G_Device *device, G_BufferKey key)
{
	return G_DeviceBufferListGet(&device->buffers, key);
}

static void *G_DeviceBufferMap(const G_Device *device, G_BufferKey key)
{
	G_Buffer *buffer = G_DeviceBufferListGet(&device->buffers, key);
	DebugLogAssert(device->log_channel, buffer, "Invalid buffer with key %llu when mapping memory.", key.value);
	
	return buffer->allocation_info.pMappedData;
}

static u64 G_DeviceBufferAddress(const G_Device *device, G_BufferKey key)
{
	G_Buffer *buffer = G_DeviceBufferListGet(&device->buffers, key);
	DebugLogAssert(device->log_channel, buffer, "Invalid buffer with key %llu when getting device address.", key.value);

	return buffer->device_address;
}

static void G_DeviceBufferRead(const G_Device *device, G_BufferKey key, void *dst, u64 length, u64 offset)
{
	G_Buffer *buffer = G_DeviceBufferListGet(&device->buffers, key);
	DebugLogAssert(device->log_channel, buffer, "Invalid buffer with key %llu when reading.", key.value);
	
	vmaCopyAllocationToMemory(device->context.vma_allocator, buffer->allocation, offset, dst, length);
}

static void G_DeviceBufferWrite(const G_Device *device, G_BufferKey key, const void *src, u64 length, u64 offset)
{
	G_Buffer *buffer = G_DeviceBufferListGet(&device->buffers, key);
	DebugLogAssert(device->log_channel, buffer, "Invalid buffer with key %llu when writing.", key.value);
	
	vmaCopyMemoryToAllocation(device->context.vma_allocator, src, buffer->allocation, offset, length);
}

static u64 G_DeviceBufferSize(const G_Device *device, G_BufferKey key)
{
	G_Buffer *buffer = G_DeviceBufferListGet(&device->buffers, key);
	DebugLogAssert(device->log_channel, buffer, "Invalid buffer with key %llu when getting size.", key.value);

	return buffer->size;
}

static G_SamplerKey G_DeviceSamplerCreate(G_Device *device, const G_SamplerCreateInfo *info)
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

	G_Sampler sampler = {0};
	
	G_VK_CHECK(vkCreateSampler(device->context.device,
							   &create_info, NULL,
							   &sampler.vk_handle),
			   "Failed to create texture sampler.");

	sampler.filter = info->filter;
	sampler.wrap_x = info->wrap_x;
	sampler.wrap_y = info->wrap_y;
	sampler.wrap_z = info->wrap_z;
	sampler.border_colour = info->border_colour;
	sampler.bindless = G_BindlessRegisterSampler(&device->bindless, sampler.vk_handle);

	return G_DeviceSamplerListPushAuto(&device->samplers, device->permanent_arena, &sampler);
}

static G_SamplerKey G_DeviceSamplerCreateF(G_Device *device, VkFilter filter)
{
	G_SamplerCreateInfo create_info = {0};
	create_info.filter = filter;
	create_info.wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.border_colour = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	return G_DeviceSamplerCreate(device, &create_info);
}

static void G_DeviceSamplerDestroy(G_Device *device, G_SamplerKey sampler_key)
{
	G_Sampler *sampler = G_DeviceSamplerListGet(&device->samplers, sampler_key);
	
	AssertTrue(sampler);

	G_DevicePerFrameData *frame_data = &device->per_frame_data[device->current_frame_index];

	G_DestroyedSampler *node = ArenaPushArray(&frame_data->arena, G_DestroyedSampler, 1);
	node->sampler = sampler->vk_handle;
	node->bindless = sampler->bindless;
	node->next = frame_data->destroyed_sampler_head;
	frame_data->destroyed_sampler_head = node;
}

static G_Sampler *G_DeviceSamplerFromKey(const G_Device *device, G_SamplerKey key)
{
	return G_DeviceSamplerListGet(&device->samplers, key);
}

static G_BindlessIndex G_DeviceSamplerBindless(const G_Device *device, G_SamplerKey key)
{
	G_Sampler *gfx_sampler = G_DeviceSamplerFromKey(device, key);
	DebugLogAssert(device->log_channel, gfx_sampler, "Invalid sampler with key %llu when destroying.", key.value);
	
	return G_BindlessIndexOf(gfx_sampler->bindless);
}

static G_ShaderStage G_DeviceShaderStageCreate(G_Device *device, Arena *arena, const G_ShaderBytecode *bytecode)
{
	SpvReflectShaderModule reflect_module = {0};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(bytecode->size, bytecode->bytes, &reflect_module);

	DebugLogAssert(device->log_channel,
				   reflect_result == SPV_REFLECT_RESULT_SUCCESS,
				   "Failed to reflect SPIR-V module: %d\n", reflect_result);
	
	ScratchArena scratch = ScratchBegin(&arena, 1);

	G_ShaderStage stage = {0};

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
	}
	else
	{
		DebugLogB(device->log_channel, "No entry points found in SPIR-V.");
	}

	spvReflectDestroyShaderModule(&reflect_module);

	ScratchRelease(&scratch);

	return stage;
}

static G_ShaderKey G_DeviceShaderProgramCreate(G_Device *device, u32 stage_count, const G_ShaderBytecode *stages)
{
	G_ShaderProgram program = {0};
	program.stage_count = stage_count;
	program.push_constant_size = 0;

	for (u32 i = 0; i < stage_count; i++)
	{
		program.stages[i] = G_DeviceShaderStageCreate(device, device->permanent_arena, &stages[i]);
		program.push_constant_size = MaxValue(program.push_constant_size, program.stages[i].push_constant_size);
	}

	static u32 shader_cookie = 0;
	program.cookie = ++shader_cookie;

	return G_DeviceShaderListPushAuto(&device->shaders, device->permanent_arena, &program);
}

static void G_DeviceShaderProgramDestroy(G_Device *device, G_ShaderKey program_key)
{
	// since we just pass the shader params directly into the pipeline state
	// when creating it we have nothing to destroy. but i'm keeping this just
	// in case, also i kinda like symmetry.
}

static G_ShaderProgram *G_DeviceShaderProgramFromKey(const G_Device *device, G_ShaderKey key)
{
	return G_DeviceShaderListGet(&device->shaders, key);
}

static G_DeviceAllocAccelStructReceipt G_DeviceBLASAlloc(G_Device *device, const G_BLASGeometry *geometries, u32 geometry_count)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkAccelerationStructureGeometryKHR *vk_geometries = ArenaPushArray(scratch.arena, VkAccelerationStructureGeometryKHR, geometry_count);
	u32 *primitive_counts = ArenaPushArray(scratch.arena, u32, geometry_count);

	for (u32 i = 0; i < geometry_count; i++)
	{
		const G_BLASGeometry *geometry = &geometries[i];

		G_Buffer *vb = G_DeviceBufferFromKey(device, geometry->vertex_buffer);
		G_Buffer *ib = G_DeviceBufferFromKey(device, geometry->index_buffer);

		vk_geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vk_geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vk_geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureGeometryTrianglesDataKHR *tri = &vk_geometries[i].geometry.triangles;

		tri->sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		tri->vertexFormat             = geometry->vertex_format;
		tri->vertexData.deviceAddress = vb->device_address + geometry->vertex_offset;
		tri->vertexStride             = geometry->vertex_stride;
		tri->maxVertex                = geometry->vertex_count - 1;

		tri->indexType                = geometry->index_type;
		tri->indexData.deviceAddress  = ib->device_address + geometry->index_offset;

		primitive_counts[i] = geometry->index_count / 3;
	}

	// so much fucking typing oh my god.
	VkAccelerationStructureBuildGeometryInfoKHR build_info = {0};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.geometryCount = geometry_count;
	build_info.pGeometries = vk_geometries;

	VkAccelerationStructureBuildSizesInfoKHR sizes = {0};
	sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	vkGetAccelerationStructureBuildSizesKHR(device->context.device,
											VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
											&build_info,
											primitive_counts,
											&sizes);

	// backing buffer also needs alloc
	
	// this feels bad 'cuz we should be automatically generating the device address
	// in the buffer alloc like we do for usual storage buffers but i figure if
	// this is the only time we do this i can get away with being lazy :p
	G_BufferAllocInfo buf_alloc_info = {0};
	buf_alloc_info.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
	buf_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buf_alloc_info.size = sizes.accelerationStructureSize;

	G_BufferKey backing_buffer_key = G_DeviceBufferAlloc(device, &buf_alloc_info);
	G_Buffer *backing_buffer = G_DeviceBufferFromKey(device, backing_buffer_key);

	VkAccelerationStructureCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.buffer = backing_buffer->vk_handle;
	create_info.size = sizes.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	G_AccelStruct accel_struct = {0};
	accel_struct.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accel_struct.backing_buffer = backing_buffer_key;

	G_VK_CHECK(vkCreateAccelerationStructureKHR(device->context.device,
												&create_info, NULL,
												&accel_struct.vk_handle),
			   "Failed to create BLAS.");

	VkAccelerationStructureDeviceAddressInfoKHR addr_info = {0}; // fuck me khronos shorter names would be appreciated yes??
	addr_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addr_info.accelerationStructure = accel_struct.vk_handle;

	accel_struct.device_address = vkGetAccelerationStructureDeviceAddressKHR(device->context.device, &addr_info);
	
	ScratchRelease(&scratch);

	DebugLogD(device->log_channel, "Allocated BLAS from %u geometries.", geometry_count);

	G_DeviceAllocAccelStructReceipt receipt = {0};
	receipt.key = G_DeviceAccelStructListPushAuto(&device->accel_structures, device->permanent_arena, &accel_struct);
	receipt.scratch_size = sizes.buildScratchSize;
	
	return receipt;
}

static G_DeviceAllocAccelStructReceipt G_DeviceTLASAlloc(G_Device *device, u32 max_instance_count)
{
	VkAccelerationStructureGeometryKHR geometry = {0};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR; // fuck you

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {0};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.geometryCount = 1;
	build_info.pGeometries = &geometry;

	VkAccelerationStructureBuildSizesInfoKHR sizes = {0};
	sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	vkGetAccelerationStructureBuildSizesKHR(device->context.device,
											VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
											&build_info,
											&max_instance_count,
											&sizes);
	
	// backing buffer also needs alloc
	
	// this feels bad cuz' we should be automatically generating the device address
	// in the buffer alloc like we do for usual storage buffers but i figure if
	// this is the only time we do this i can get away with being lazy :p
	G_BufferAllocInfo buf_alloc_info = {0};
	buf_alloc_info.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
	buf_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buf_alloc_info.size = sizes.accelerationStructureSize;

	G_BufferKey backing_buffer_key = G_DeviceBufferAlloc(device, &buf_alloc_info);
	G_Buffer *backing_buffer = G_DeviceBufferFromKey(device, backing_buffer_key);

	VkAccelerationStructureCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.buffer = backing_buffer->vk_handle;
	create_info.size = sizes.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	G_AccelStruct accel_struct = {0};
	accel_struct.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accel_struct.backing_buffer = backing_buffer_key;

	G_VK_CHECK(vkCreateAccelerationStructureKHR(device->context.device,
												&create_info, NULL,
												&accel_struct.vk_handle),
			   "Failed to create TLAS.");

	VkAccelerationStructureDeviceAddressInfoKHR addr_info = {0}; // fuck me khronos shorter names would be appreciated yes??
	addr_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addr_info.accelerationStructure = accel_struct.vk_handle;

	accel_struct.device_address = vkGetAccelerationStructureDeviceAddressKHR(device->context.device, &addr_info);

	DebugLogD(device->log_channel, "Allocated TLAS with max instance count %u.", max_instance_count);
	
	G_DeviceAllocAccelStructReceipt receipt = {0};
	receipt.key = G_DeviceAccelStructListPushAuto(&device->accel_structures, device->permanent_arena, &accel_struct);
	receipt.scratch_size = sizes.buildScratchSize;
	
	return receipt;
}

static void G_DeviceAccelStructDestroy(G_Device *device, G_AccelStructKey key)
{
	G_AccelStruct *accel_struct = G_DeviceAccelStructListGet(&device->accel_structures, key);
	DebugLogAssert(device->log_channel, accel_struct, "Invalid acceleration structure with key %llu when destroying.", key.value);

	vkDestroyAccelerationStructureKHR(device->context.device, accel_struct->vk_handle, NULL);
	accel_struct->vk_handle = VK_NULL_HANDLE;

	G_DeviceBufferDestroy(device, accel_struct->backing_buffer);
}

static u64 G_DeviceAccelStructAddress(G_Device *device, G_AccelStructKey key)
{
	G_AccelStruct *accel_struct = G_DeviceAccelStructListGet(&device->accel_structures, key);
	DebugLogAssert(device->log_channel, accel_struct, "Invalid acceleration structure with key %llu when getting device address.", key.value);

	return accel_struct->device_address;
}

static G_AccelStruct *G_DeviceAccelStructFromKey(G_Device *device, G_AccelStructKey key)
{
	return G_DeviceAccelStructListGet(&device->accel_structures, key);
}

static void G_DeviceCreateSyncResources(G_Device *device)
{
	device->graphics_semaphore = G_DeviceSemaphoreCreate(device, 0);

	u32 family_index = device->context.graphics_queue.family_index;

	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
	{
		device->per_frame_data[i].completion_point.value = 0;
		device->per_frame_data[i].completion_point.semaphore = device->graphics_semaphore.vk_handle;

		device->per_frame_data[i].command_pool = G_DeviceCmdPoolCreate(device, family_index);

		VkSemaphoreCreateInfo binary_semaphore_create_info = {0};
		binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		binary_semaphore_create_info.flags = 0;
		binary_semaphore_create_info.pNext = NULL;

		G_VK_CHECK(vkCreateSemaphore(device->context.device,
									 &binary_semaphore_create_info, NULL,
									 &device->per_frame_data[i].image_available_semaphore),
				   "Failed to create image available semaphore.");

		G_VK_CHECK(vkCreateSemaphore(device->context.device,
									 &binary_semaphore_create_info, NULL,
									 &device->per_frame_data[i].render_finished_semaphore),
				   "Failed to create render finished semaphore.");

		device->per_frame_data[i].destroyed_sampler_head = NULL;
		device->per_frame_data[i].destroyed_image_head = NULL;
		//device->per_frame_data[i].destroyed_view_head = NULL;
		device->per_frame_data[i].destroyed_buffer_head = NULL;
	}

	DebugLogD(device->log_channel, "Created frame sync objects.");
}

static void G_DeviceDestroySyncResources(G_Device *device)
{
	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(device->context.device, device->per_frame_data[i].image_available_semaphore, NULL);
		vkDestroySemaphore(device->context.device, device->per_frame_data[i].render_finished_semaphore, NULL);
		G_DeviceCmdPoolDestroy(device, &device->per_frame_data[i].command_pool);
		G_DeviceFlushFrameData(device, &device->per_frame_data[i]);
	}

	G_DeviceSemaphoreDestroy(device, &device->graphics_semaphore);
}

static void G_DeviceCreateBindless(G_Device *device)
{
	VkDescriptorPoolSize pool_sizes[G_BindlessKind_COUNT] = {0};

	for (u32 i = 0; i < G_BindlessKind_COUNT; i++)
	{
		pool_sizes[i].type = G_BindlessGetVkType((G_BindlessKind)i);
		pool_sizes[i].descriptorCount = G_BINDLESS_MAX_RESOURCES;
	}

	VkDescriptorPoolCreateInfo pool_create_info = {0};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = ArraySize(pool_sizes) * G_BINDLESS_MAX_RESOURCES;
	pool_create_info.poolSizeCount = ArraySize(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;

	G_VK_CHECK(vkCreateDescriptorPool(device->context.device,
									  &pool_create_info, NULL,
									  &device->bindless.pool),
			   "Failed to create bindless descriptor pool.");

	VkDescriptorSetLayoutBinding bindings[G_BindlessKind_COUNT] = {0};
	VkDescriptorBindingFlags bindless_flags[G_BindlessKind_COUNT] = {0};

	for (u32 i = 0; i < G_BindlessKind_COUNT; i++)
	{
		bindings[i].descriptorType = G_BindlessGetVkType((G_BindlessKind)i);
		bindings[i].descriptorCount = G_BINDLESS_MAX_RESOURCES;
		bindings[i].binding = i;
		bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
		bindings[i].pImmutableSamplers = NULL;
		
		bindless_flags[i] =
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
	}
	
	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {0};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = ArraySize(bindless_flags);
	binding_flags.pBindingFlags = bindless_flags;

	VkDescriptorSetLayoutCreateInfo layout_create_info = {0};
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = ArraySize(bindings);
	layout_create_info.pBindings = bindings;
	layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
	layout_create_info.pNext = &binding_flags;

	G_VK_CHECK(vkCreateDescriptorSetLayout(device->context.device,
										   &layout_create_info, NULL,
										   &device->bindless.layout),
			   "Failed to create bindless descriptor layout.");

	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = device->bindless.pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &device->bindless.layout;

	G_VK_CHECK(vkAllocateDescriptorSets(device->context.device,
										&alloc_info,
										&device->bindless.set),
			   "Failed to allocate bindless descriptor set.");

	DebugLogD(device->log_channel, "Bindless resources created.");
}

static void G_DeviceDestroyBindless(G_Device *device)
{
	vkDestroyDescriptorSetLayout(device->context.device, device->bindless.layout, NULL);
	vkDestroyDescriptorPool(device->context.device, device->bindless.pool, NULL);
}

static void G_DeviceApplyBindlessUpdates(G_Device *device)
{
	if (device->bindless.update_count == 0)
		return;

	ScratchArena scratch = ScratchBegin(NULL, 0);

	u32 count = device->bindless.update_count;

	VkWriteDescriptorSet *writes = ArenaPushArray(scratch.arena, VkWriteDescriptorSet, count);
	VkDescriptorImageInfo *infos = ArenaPushArray(scratch.arena, VkDescriptorImageInfo, count);

	for (u32 i = 0; i < count; i++)
	{
		G_BindlessUpdate *update = &device->bindless.updates[i];

		infos[i].sampler = update->sampler;
		infos[i].imageView = update->view;
		infos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		writes[i] = (VkWriteDescriptorSet) {0};
		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].descriptorCount = 1;
		writes[i].dstArrayElement = update->slot;
		writes[i].descriptorType = G_BindlessGetVkType(update->kind);
		writes[i].dstSet = device->bindless.set;
		writes[i].dstBinding = update->kind;
		writes[i].pImageInfo = &infos[i];
	}

	vkUpdateDescriptorSets(device->context.device,
						   count, writes,
						   0, NULL);

	device->bindless.update_count = 0;

	ScratchRelease(&scratch);
}

static void G_DeviceCreateImGui(G_Device *device)
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

	  G_VK_CHECK(vkCreateDescriptorPool(device->context.device,
	  &pool_info, NULL,
	  &device->imgui_pool),
	  "Failed to create ImGui descriptor pool.");

	  VkFormat swapchain_image_format = VK_FORMAT_R32G32B32A32_SFLOAT;

	  ImGui_ImplVulkan_InitInfo init_info = {0};
	  init_info.Instance = device->context.instance;
	  init_info.PhysicalDevice = device->context.physical_device;
	  init_info.Device = device->context.device;
	  init_info.QueueFamily = device->context.graphics_queue.family_index;
	  init_info.Queue = device->context.graphics_queue.vk_handle;
	  init_info.PipelineCache = device->pipeline_cache;
	  init_info.DescriptorPool = device->imgui_pool;
	  init_info.Allocator = NULL;
	  init_info.MinImageCount = G_FRAMES_IN_FLIGHT;
	  init_info.ImageCount = G_FRAMES_IN_FLIGHT;
	  init_info.CheckVkResultFn = NULL;
	  init_info.UseDynamicRendering = true;
	  init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_image_format;

	  ImGui_ImplVulkan_Init(&init_info);
	*/
}

static void G_DeviceDestroyImGui(G_Device *device)
{
	/*
	  ImGui_ImplVulkan_Shutdown();
	  vkDestroyDescriptorPool(device->context.device, device->imgui_pool, NULL);
	*/
}

static void G_DeviceImGuiNewFrame(const G_Device *device)
{
	/*
	  ImGui_ImplVulkan_NewFrame();
	*/
}

static void G_DeviceImGuiRecord(const G_Device *device, const G_CmdBuffer *cmd)
{
	/*
	  ImDrawData *draw_data = ImGui_GetDrawData();
	  ImGui_ImplVulkan_RenderDrawData(draw_data, cmd->vk_handle);
	*/
}
