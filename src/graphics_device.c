
global const char *GRAPHICS_VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation"
};

internal VkFormat FindGraphicsSupportedFormat(VkPhysicalDevice physical_device,
					      VkImageTiling tiling,
					      VkFormatFeatureFlags features,
					      u32 candidate_count,
					      VkFormat *candidates)
{
	for (i32 i = 0; i < candidate_count; i++) {
		VkFormatProperties properties = {0};
		vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) ||
		    (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)) {
			return candidates[i];
		}
	}
	
	DebugLogCrash("Failed to find supported format.");

	return VK_FORMAT_MAX_ENUM;
}

internal VkFormat FindGraphicsDepthFormat(VkPhysicalDevice physical_device)
{
	static VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	return FindGraphicsSupportedFormat(physical_device, VK_IMAGE_TILING_OPTIMAL,
					   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
					   ArraySize(candidates), candidates);
}

internal VkSampleCountFlagBits FindGraphicsMaxUsableSampleCount(VkPhysicalDeviceProperties2 properties)
{
	VkSampleCountFlags counts =
		properties.properties.limits.framebufferColorSampleCounts &
		properties.properties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT)      return VK_SAMPLE_COUNT_64_BIT;
	else if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
	else if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
	else if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
	else if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
	else if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;
	else if (counts & VK_SAMPLE_COUNT_1_BIT)  return VK_SAMPLE_COUNT_1_BIT;

	DebugLogCrash("Could not find a maximum usable sample count.");
	
	return VK_SAMPLE_COUNT_1_BIT;
}

internal void GraphicsWaitIdle()
{
	vkDeviceWaitIdle(graphics_device->device);
}

internal void WaitForFence(VkFence fence)
{
	vkWaitForFences(graphics_device->device, 1, &fence, VK_TRUE, UINT64_MAX);
}

internal void ResetFence(VkFence fence)
{
	vkResetFences(graphics_device->device, 1, &fence);
}

internal const char *const *GetInstanceExtensions(MemoryArena *arena, u32 *extension_count)
{
	const char *const *names = platform->GetVulkanInstanceExtensions(extension_count);

	if (!names)
		DebugLogCrash("Unable to get instance extension count.");

	u32 extra_extension_count = 3;

#ifdef __APPLE__
	extra_extension_count += 2;
#endif

	const char **extensions = MemoryArenaPushC(arena, sizeof(const char *), *extension_count + extra_extension_count);

	for (i32 i = 0; i < *extension_count; i++)
		extensions[i] = names[i];

	extensions[*extension_count + 0] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
	extensions[*extension_count + 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	extensions[*extension_count + 2] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

#ifdef __APPLE__
	extensions[*extension_count + 3] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	extensions[*extension_count + 4] = "VK_EXT_metal_surface";
#endif

	*extension_count = *extension_count + extra_extension_count;

	return extensions;
}

internal b32 CheckGraphicsPhysicalDeviceExtensionSupport(MemoryArena *arena, VkPhysicalDevice physical_device)
{
	u32 extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, 0,
					     &extension_count, 0);

	if (extension_count <= 0)
		DebugLogCrash("Failed to find any device extension properties.");

	ScratchArena scratch = GetScratch(arena, 1);

	VkExtensionProperties *available_exts = MemoryArenaPushC(scratch.arena,
								sizeof(VkExtensionProperties),
								extension_count);
	
	vkEnumerateDeviceExtensionProperties(physical_device, 0, &extension_count, available_exts);

	b32 result = true;

	for (i32 i = 0; i < extension_count; i++) {
		for (i32 j = 0; j < ArraySize(GRAPHICS_VALIDATION_LAYERS); j++) {
			if (CStringCompare(available_exts[i].extensionName, GRAPHICS_VALIDATION_LAYERS[j]) == 0) {
				result = false;
				goto exit;
			}
		}
	}

exit:
	ReleaseScratch(&scratch);
	return result;
}

// fucking sucks.
internal u32 AssignGraphicsPhysicalDeviceUsability(MemoryArena *arena, VkSurfaceKHR surface,
						   VkPhysicalDevice physical_device,
						   VkPhysicalDeviceProperties2 properties,
						   VkPhysicalDeviceFeatures2 features, b32 *has_essentials)
{
	u32 usability = 0;

	b32 adequate_swap_chain = false;
	b32 has_required_extensions = CheckGraphicsPhysicalDeviceExtensionSupport(arena, physical_device);
	b32 has_anisotropy = features.features.samplerAnisotropy;

	// Prefer / give more weight to discrete gpus than integrated gpus.
	if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		usability += 4;
	else if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		usability += 1;

	// If we have anisotropy then that's good... I guess :))).
	if (has_anisotropy)
		usability += 1;

	// It must have the required extensions.
	if (has_required_extensions) {
		ScratchArena scratch = GetScratch(arena, 1);

		SwapchainSupportDetails details = QuerySwapchainSupport(scratch.arena, physical_device, surface);

		adequate_swap_chain =
			(details.surface_format_count > 0) &&
			(details.present_mode_count > 0);

		usability += 3;

		ReleaseScratch(&scratch);
	}

	// Essential features must be satisfied.
	if (has_essentials) {
		*has_essentials =
			has_required_extensions &&
			adequate_swap_chain &&
			has_anisotropy;
	}

	return usability;
}

internal b32 CheckForValidationLayerSupport(MemoryArena *arena)
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);

	ScratchArena scratch = GetScratch(arena, 1);

	VkLayerProperties *available_layers = MemoryArenaPush(scratch.arena, sizeof(VkLayerProperties) * layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	b32 result = true;
	
	for (i32 i = 0; i < ArraySize(GRAPHICS_VALIDATION_LAYERS); i++) {
		b32 has_layer = false;
		const char *layer_name_0 = GRAPHICS_VALIDATION_LAYERS[i];

		for (i32 j = 0; j < layer_count; j++) {
			const char *layer_name_1 = available_layers[j].layerName;

			if (CStringCompare(layer_name_0, layer_name_1) == 0) {
				has_layer = true;
				break;
			}
		}

		if (!has_layer) {
			result = false;
			goto exit;
		}
	}

exit:
	ReleaseScratch(&scratch);
	return result;
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsVulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
								    VkDebugUtilsMessageTypeFlagsEXT message_type,
								    const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
								    void *p_user_data)
{
	if (message_severity >=
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		DebugLogCrash("Severity = %d, Type = %d, Message = \"%s\"",
			      message_severity, message_type,
			      p_callback_data->pMessage);
	}

	return VK_FALSE;
}

internal VkResult CreateGraphicsDeviceDebugUtilsMessengerExt(VkInstance instance,
							     VkDebugUtilsMessengerCreateInfoEXT *debug_info,
							     const VkAllocationCallbacks *allocator,
							     VkDebugUtilsMessengerEXT *messenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn)
		return fn(instance, debug_info, allocator, messenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// TODO: We really shouldn't need to initialize
//       and de-initialize volk like this. Surely
//       we should load all fpointers into a table
//       which we store in program memory (possible
//       in volk) and just load that table back
//       in after reloading?

// TODO: Also I'm 99% certain that VMA's function
//       pointers also break down here, and I'm
//       not setting them back... Need to test.

internal void GraphicsDeviceBeforeHotReload()
{
	volkFinalize();
}

internal void GraphicsDeviceAfterHotReload()
{
	volkInitialize();
	volkLoadInstance(graphics_device->instance);
	volkLoadDevice(graphics_device->device);

	GraphicsWaitIdle();
}

internal void GraphicsDeviceInit(MemoryArena *arena)
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

	ScratchArena scratch = GetScratch(arena, 1);

	instance_create_info.ppEnabledExtensionNames = GetInstanceExtensions(scratch.arena, &instance_create_info.enabledExtensionCount);

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
	debug_create_info.pUserData = NULL;

	graphics_device->has_validation_layers = CheckForValidationLayerSupport(scratch.arena);

	if (graphics_device->has_validation_layers) {
		DebugLog("Validation layer support verified.");

		instance_create_info.enabledLayerCount = ArraySize(GRAPHICS_VALIDATION_LAYERS);
		instance_create_info.ppEnabledLayerNames = GRAPHICS_VALIDATION_LAYERS;
		instance_create_info.pNext = &debug_create_info;
	} else {
		DebugLog("No validation layer support.");

		instance_create_info.enabledLayerCount = 0;
		instance_create_info.ppEnabledLayerNames = NULL;
		instance_create_info.pNext = NULL;
	}

#ifdef __APPLE__
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &graphics_device->instance),
		 "Failed to create instance.");

	volkLoadInstance(graphics_device->instance);

	if (graphics_device->has_validation_layers) {
		VK_CHECK(CreateGraphicsDeviceDebugUtilsMessengerExt(graphics_device->instance, &debug_create_info, NULL,
								    &graphics_device->debug_messenger),
			 "Failed to create debug messenger.");
	}

	if (!platform->CreateVulkanSurface((void *)graphics_device->instance, (void *)&graphics_device->surface))
		DebugLogCrash("Failed to create surface.");

	// Enumerate physical devices.
	{
		u32 device_count = 0;
		vkEnumeratePhysicalDevices(graphics_device->instance, &device_count, NULL);

		if (device_count <= 0)
			DebugLogCrash("Failed to find GPUs with Vulkan support.");

		VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		VkPhysicalDeviceFeatures2 features     = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

		VkPhysicalDevice *devices = MemoryArenaPushC(scratch.arena, sizeof(VkPhysicalDevice), device_count);
		
		vkEnumeratePhysicalDevices(graphics_device->instance,
					   &device_count, devices);

		vkGetPhysicalDeviceProperties2(devices[0], &properties);
		vkGetPhysicalDeviceFeatures2(devices[0], &features);

		graphics_device->physical_device = devices[0];
		graphics_device->physical_device_properties = properties;
		graphics_device->physical_device_features = features;

		b32 has_essentials = false;

		u32 usability0 = AssignGraphicsPhysicalDeviceUsability(scratch.arena,
								       graphics_device->surface,
								       graphics_device->physical_device,
								       properties, features,
								       &has_essentials);

		u32 selected_index = 0;

		for (i32 i = 0; i < device_count; i++) {
			vkGetPhysicalDeviceProperties2(devices[i], &graphics_device->physical_device_properties);
			vkGetPhysicalDeviceFeatures2  (devices[i], &graphics_device->physical_device_features);

			u32 usability1 = AssignGraphicsPhysicalDeviceUsability(scratch.arena,
									       graphics_device->surface,
									       devices[i],
									       properties, features,
									       &has_essentials);

			if (usability1 > usability0 && has_essentials) {
				usability0 = usability1;

				graphics_device->physical_device = devices[i];
				graphics_device->physical_device_properties = properties;
				graphics_device->physical_device_features = features;

				selected_index = i;
			}

			if (!graphics_device->physical_device)
				DebugLogCrash("Unable to find a suitable GPU.");

			DebugLog("Selected a suitable GPU: %d", selected_index);
		}
	}

	graphics_device->max_msaa_samples = FindGraphicsMaxUsableSampleCount(graphics_device->physical_device_properties);
	graphics_device->depth_format = FindGraphicsDepthFormat(graphics_device->physical_device);

	// Locate the graphics queue.
	{
		u32 queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(graphics_device->physical_device, &queue_family_count, 0);

		if (queue_family_count <= 0)
			DebugLogCrash("Failed to find any queue families.");

		VkQueueFamilyProperties *queue_families = MemoryArenaPushC(scratch.arena, sizeof(VkQueueFamilyProperties), queue_family_count);
		
		vkGetPhysicalDeviceQueueFamilyProperties(graphics_device->physical_device,
							 &queue_family_count,
							 queue_families);

		for (i32 i = 0; i < queue_family_count; i++) {
			if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
				b32 present_support = false;

				vkGetPhysicalDeviceSurfaceSupportKHR(graphics_device->physical_device, i,
								     graphics_device->surface,
								     &present_support);

				if (present_support) {
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

	// Disable this so we get a clear indication if something's gone wrong.
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
	device_create_info.ppEnabledLayerNames = NULL;
	device_create_info.enabledExtensionCount = ArraySize(GRAPHICS_DEVICE_EXTENSIONS);
	device_create_info.ppEnabledExtensionNames = GRAPHICS_DEVICE_EXTENSIONS;
	device_create_info.pEnabledFeatures = &graphics_device->physical_device_features.features;
	device_create_info.pNext = &vulkan13_features;

	if (graphics_device->has_validation_layers) {
		device_create_info.enabledLayerCount = ArraySize(GRAPHICS_VALIDATION_LAYERS);
		device_create_info.ppEnabledLayerNames = GRAPHICS_VALIDATION_LAYERS;

		DebugLog("Enabled validation layers.");
	}

	VK_CHECK(vkCreateDevice(graphics_device->physical_device,
				&device_create_info, NULL,
				&graphics_device->device),
		 "Failed to create logical device.");

	vkGetDeviceQueue(graphics_device->device,
			 graphics_device->graphics_queue_family_index, 0,
			 &graphics_device->graphics_queue);

	DebugLog("Created logical device.");

	VkSemaphoreCreateInfo semaphore_create_info = {0};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
		CommandPoolInit(&graphics_device->frames[i].command_pool, graphics_device->graphics_queue_family_index);

		VkFenceCreateInfo in_flight_fence_create_info = {0};
		in_flight_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		in_flight_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VK_CHECK(vkCreateFence(graphics_device->device,
				       &in_flight_fence_create_info, NULL,
				       &graphics_device->frames[i].in_flight_fence),
			 "Failed to create queue frame in flight fence.");

		VkFenceCreateInfo instant_submit_fence_create_info = {0};
		instant_submit_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		instant_submit_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VK_CHECK(vkCreateFence(graphics_device->device,
				       &instant_submit_fence_create_info, NULL,
				       &graphics_device->frames[i].instant_submit_fence),
			 "Failed to create queue frame instant submit fence.");

		VK_CHECK(vkCreateSemaphore(graphics_device->device,
					   &semaphore_create_info, NULL,
					   &graphics_device->frames[i].image_available_semaphore),
			 "Failed to create image available semaphore.");

		VK_CHECK(vkCreateSemaphore(graphics_device->device,
					   &semaphore_create_info, NULL,
					   &graphics_device->frames[i] .render_finished_semaphore),
			 "Failed to create render finished semaphore.");
	}

	DebugLog("Created frame sync objects.");

	u32 version = 0;
	VkResult result = vkEnumerateInstanceVersion(&version);

	if (result == VK_SUCCESS) {
		u32 major = VK_API_VERSION_MAJOR(version);
		u32 minor = VK_API_VERSION_MINOR(version);

		DebugLog("Using Vulkan %d.%d", major, minor);
	} else {
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

	VK_CHECK(vkCreatePipelineCache(graphics_device->device,
				       &pipeline_cache_create_info, NULL,
				       &graphics_device->pipeline_process_cache),
		 "Failed to process pipeline cache.");

	DebugLog("Created graphics pipeline process cache.");

	SwapchainInit(&graphics_device->swapchain, arena);
	BindlessInit(&graphics_device->bindless);

	HashTableInit(&graphics_device->image_view_cache,      arena, sizeof(ImageView));
	HashTableInit(&graphics_device->pipeline_cache,        arena, sizeof(VkPipeline));
	HashTableInit(&graphics_device->pipeline_layout_cache, arena, sizeof(VkPipelineLayout));

	DebugLog("Created graphics object caches.");

	ReleaseScratch(&scratch);
}

internal void GraphicsDeviceDestroy()
{
	GraphicsWaitIdle();

	// Destroy cached image views.
	for (i32 i = 0; i < ArraySize(graphics_device->image_view_cache.buckets); i++) {
		if (graphics_device->image_view_cache.buckets[i]) {
			HashTableNode *node = graphics_device->image_view_cache.buckets[i];

			while (node) {
				ImageViewDestroy((ImageView *)node->data);
				node = node->next;
			}
		}
	}

	// Destroy cached pipeline layouts.
	for (i32 i = 0; i < ArraySize(graphics_device->pipeline_layout_cache.buckets); i++) {
		if (graphics_device->pipeline_layout_cache.buckets[i]) {
			HashTableNode *node = graphics_device->pipeline_layout_cache.buckets[i];

			while (node) {
				PipelineLayoutDestroy(*((VkPipelineLayout *)node->data));
				node = node->next;
			}
		}
	}

	// Destroy cached pipelines.
	for (i32 i = 0; i < ArraySize(graphics_device->pipeline_cache.buckets); i++) {
		if (graphics_device->pipeline_cache.buckets[i]) {
			HashTableNode *node = graphics_device->pipeline_cache.buckets[i];

			while (node) {
				PipelineDestroy(*((VkPipeline *)node->data));
				node = node->next;
			}
		}
	}

	// Clean up frame synchronization objects.
	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
		CommandPoolDestroy(&graphics_device->frames[i].command_pool);

		vkDestroyFence(graphics_device->device, graphics_device->frames[i].in_flight_fence, NULL);
		vkDestroyFence(graphics_device->device, graphics_device->frames[i].instant_submit_fence, NULL);

		vkDestroySemaphore(graphics_device->device, graphics_device->frames[i].render_finished_semaphore, NULL);
		vkDestroySemaphore(graphics_device->device, graphics_device->frames[i].image_available_semaphore, NULL);
	}

	BindlessDestroy(&graphics_device->bindless);
	SwapchainDestroy(&graphics_device->swapchain);

	vkDestroyPipelineCache(graphics_device->device, graphics_device->pipeline_process_cache, NULL);
	vkDestroySurfaceKHR(graphics_device->instance, graphics_device->surface, NULL);
	vmaDestroyAllocator(graphics_device->vma_allocator);
	vkDestroyDebugUtilsMessengerEXT(graphics_device->instance, graphics_device->debug_messenger, NULL);
	vkDestroyDevice(graphics_device->device, NULL);
}

internal CommandBuffer BeginGraphicsPresent()
{
	GraphicsFrameData *current_frame = graphics_device->frames + graphics_device->current_frame_index;

	WaitForFence(current_frame->in_flight_fence);
	ResetFence(current_frame->in_flight_fence);

	SwapchainAcquireNextImage(&graphics_device->swapchain);

	CommandBuffer in_flight_cmd = CommandPoolFetchFreeBuffer(&current_frame->command_pool);

	CmdBegin(&in_flight_cmd);

	return in_flight_cmd;
}

internal void EndGraphicsPresent(CommandBuffer *in_flight_cmd)
{
	BindlessApplyUpdates(&graphics_device->bindless);
	
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
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;

	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;

	submit_info.signalSemaphoreInfoCount = 1;
	submit_info.pSignalSemaphoreInfos = &render_finished_semaphore;

	submit_info.waitSemaphoreInfoCount = 1;
	submit_info.pWaitSemaphoreInfos = &image_available_semaphore;

	VK_CHECK(vkQueueSubmit2(graphics_device->graphics_queue, 1,
				&submit_info, fence),
		 "Failed to submit in-flight draw command to buffer.");

	u32 image_index = graphics_device->swapchain.current_image_index;

	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = NULL;

	present_info.pImageIndices = &image_index;

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphore.semaphore;

	present_info.swapchainCount = 1;
	present_info.pSwapchains = &graphics_device->swapchain.handle;

	VkResult result = vkQueuePresentKHR(graphics_device->graphics_queue,
					    &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		DebugLogCrash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		DebugLogCrash("Failed to present swapchain image.");

	graphics_device->current_frame_index = (graphics_device->current_frame_index + 1) % FRAMES_IN_FLIGHT;

	vkQueueWaitIdle(graphics_device->graphics_queue);

	CommandPoolReset(&graphics_device->frames[graphics_device->current_frame_index].command_pool);
}

internal CommandBuffer BeginGraphicsInstantSubmit()
{
	GraphicsFrameData *current_frame = graphics_device->frames + graphics_device->current_frame_index;

	WaitForFence(current_frame->instant_submit_fence);
	ResetFence(current_frame->instant_submit_fence);

	CommandBuffer instant_submit_cmd = CommandPoolFetchFreeBuffer(&current_frame->command_pool);

	CmdBegin(&instant_submit_cmd);

	return instant_submit_cmd;
}

internal void EndGraphicsInstantSubmit(CommandBuffer *instant_submit_cmd)
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
	submit_info.pSignalSemaphoreInfos = NULL;
	
	submit_info.waitSemaphoreInfoCount = 0;
	submit_info.pWaitSemaphoreInfos = NULL;

	VK_CHECK(vkQueueSubmit2(graphics_device->graphics_queue, 1, &submit_info, fence),
		 "Failed to submit instant draw command to buffer.");
}
