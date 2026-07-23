
static const char *g_context_vk_validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};

internal VkFormat G_ContextFindGraphicsSupportedFormat(VkPhysicalDevice physical_device,
													 VkImageTiling tiling,
													 VkFormatFeatureFlags features,
													 u32 candidate_count, const VkFormat *candidates,
													 LOG_Channel log_channel)
{
	for (u32 i = 0; i < candidate_count; i++)
	{
		VkFormatProperties properties = {0};
		
		vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) ||
			(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features))
			return candidates[i];
	}

	DebugLogB(log_channel, "Failed to find supported format.");

	return VK_FORMAT_MAX_ENUM;
}

internal VkFormat G_ContextFindGraphicsDepthFormat(VkPhysicalDevice physical_device, LOG_Channel log_channel)
{
	static const VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	return G_ContextFindGraphicsSupportedFormat(physical_device,
												VK_IMAGE_TILING_OPTIMAL,
												VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
												ArraySize(candidates), candidates,
												log_channel);
}

internal VkSampleCountFlagBits G_ContextFindGraphicsMaxUsableSampleCount(VkPhysicalDeviceProperties2 properties, LOG_Channel log_channel)
{
	VkSampleCountFlags counts =
		properties.properties.limits.framebufferColorSampleCounts &
		properties.properties.limits.framebufferDepthSampleCounts;

	if      (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
	else if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
	else if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
	else if (counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
	else if (counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
	else if (counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;
	else if (counts & VK_SAMPLE_COUNT_1_BIT)  return VK_SAMPLE_COUNT_1_BIT;

	DebugLogB(log_channel, "Could not find a maximum usable sample count.");

	return VK_SAMPLE_COUNT_1_BIT;
}

internal const char * const *G_ContextGetInstanceExtensions(Arena *arena, u32 *extension_count, LOG_Channel log_channel)
{
	const char * const *names = osapi->VulkanGetInstanceExtensions(extension_count);

	if (!names)
		DebugLogB(log_channel, "Unable to get instance extension count.");

	u32 extra_extension_count = 3;

#ifdef __APPLE__
	extra_extension_count += 1;
#endif

	const char **extensions = ArenaPushArray(arena, const char *, *extension_count + extra_extension_count);

	for (u32 i = 0; i < *extension_count; i++)
		extensions[i] = names[i];

	extensions[*extension_count + 0] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
	extensions[*extension_count + 1] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	extensions[*extension_count + 2] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

#ifdef __APPLE__
	extensions[*extension_count + 3] = "VK_EXT_metal_surface";//VK_EXT_METAL_SURFACE_EXTENSION_NAME
	// using kosmickrisp
	//extensions[*extension_count + 4] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
#endif

	*extension_count = *extension_count + extra_extension_count;

	return extensions;
}

internal b32 G_ContextCheckForValidationLayerSupport(void)
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);

	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkLayerProperties *available_layers = ArenaPushArray(scratch.arena, VkLayerProperties, layer_count);
	
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	b32 result = true;

	for (u32 i = 0; i < ArraySize(g_context_vk_validation_layers); i++)
	{
		b32 has_layer = false;
		const char *layer_name_0 = g_context_vk_validation_layers[i];

		for (u32 j = 0; j < layer_count; j++)
		{
			const char *layer_name_1 = available_layers[j].layerName;

			if (CStrCompare(layer_name_0, layer_name_1) == 0)
			{
				has_layer = true;
				break;
			}
		}

		if (!has_layer)
		{
			result = false;
			goto exit;
		}
	}

exit:
	ScratchRelease(&scratch);
	return result;
}

internal G_SwapchainSupportDetails G_ContextQuerySwapchainSupport(Arena *arena,
																VkPhysicalDevice physical_device,
																VkSurfaceKHR surface)
{
	G_SwapchainSupportDetails result = {0};

	u32 surface_format_count = 0;
	u32 present_mode_count = 0;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &result.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, NULL);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);

	if (surface_format_count > 0)
	{
		result.surface_formats = ArenaPushArray(arena, VkSurfaceFormatKHR, surface_format_count);
		result.surface_format_count = surface_format_count;

		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
											 &surface_format_count,
											 result.surface_formats);
	}

	if (present_mode_count > 0)
	{
		result.present_modes = ArenaPushArray(arena, VkPresentModeKHR, present_mode_count);
		result.present_mode_count = present_mode_count;
		
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
												  &present_mode_count,
												  result.present_modes);
	}

	return result;
}

internal VkResult G_ContextCreateDeviceDebugUtilsMessengerExt(VkInstance instance,
															  VkDebugUtilsMessengerCreateInfoEXT *debug_info,
															  const VkAllocationCallbacks *allocator,
															  VkDebugUtilsMessengerEXT *messenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn)
		return fn(instance, debug_info, allocator, messenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

internal G_Context G_ContextInit(Arena *arena,
								 LOG_Channel log_channel,
								 const G_Requirements *requirements,
								 PFN_vkDebugUtilsMessengerCallbackEXT vk_debug_callback, void *vk_debug_callback_ctx)
{
	G_Context context = {0};
	
	VkApplicationInfo core_info = {0};
	
	core_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	
	core_info.pApplicationName = OS_DEFAULT_WINDOW_TITLE;
	core_info.pEngineName = OS_ENGINE_NAME;
	
	core_info.applicationVersion = VK_MAKE_API_VERSION(0,
													   OS_APP_VERSION_MAJOR,
													   OS_APP_VERSION_MINOR,
													   OS_APP_VERSION_PATCH);

	core_info.engineVersion = VK_MAKE_API_VERSION(0,
												  OS_ENGINE_VERSION_MAJOR,
												  OS_ENGINE_VERSION_MINOR,
												  OS_ENGINE_VERSION_PATCH);

	core_info.apiVersion = VK_API_VERSION_1_3;//VK_API_VERSION_1_4

	VkInstanceCreateInfo instance_create_info = {0};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &core_info;

	volkInitialize();

	ScratchArena scratch = ScratchBegin(&arena, 1);

	instance_create_info.ppEnabledExtensionNames = G_ContextGetInstanceExtensions(scratch.arena, &instance_create_info.enabledExtensionCount, log_channel);

	internal const VkValidationFeatureEnableEXT enabled_features[] = {
		//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT
	};

	VkValidationFeaturesEXT validation_features = {0};
	validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validation_features.enabledValidationFeatureCount = ArraySize(enabled_features);
	validation_features.pEnabledValidationFeatures = enabled_features;
	
	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
	debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;	
	debug_create_info.pNext = &validation_features;

	debug_create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	debug_create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	debug_create_info.pfnUserCallback = vk_debug_callback;
	debug_create_info.pUserData = vk_debug_callback_ctx;
	
	context.has_validation_layers = G_ContextCheckForValidationLayerSupport();

	if (context.has_validation_layers)
	{
		DebugLogD(log_channel, "Validation layer support verified.");

		instance_create_info.enabledLayerCount = ArraySize(g_context_vk_validation_layers);
		instance_create_info.ppEnabledLayerNames = g_context_vk_validation_layers;
		instance_create_info.pNext = &debug_create_info;
	}
	else
	{
		DebugLogW(log_channel, "No validation layer support.");

		instance_create_info.enabledLayerCount = 0;
		instance_create_info.ppEnabledLayerNames = NULL;
		instance_create_info.pNext = NULL;
	}

	/* using kosmickrisp
#ifdef __APPLE__
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
	*/
	
	G_VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &context.instance),
			   "Failed to create instance.");

	volkLoadInstance(context.instance);

	if (context.has_validation_layers)
	{
		G_VK_CHECK(G_ContextCreateDeviceDebugUtilsMessengerExt(context.instance,
															   &debug_create_info, NULL,
															   &context.debug_messenger),
				   "Failed to create debug messenger.");
	}

	if (!osapi->VulkanSurfaceCreate(context.instance, &context.surface))
		DebugLogB(log_channel, "Failed to create surface.");

	// Enumerate physical_resource devices.
	{
		u32 physical_device_count = 0;
		vkEnumeratePhysicalDevices(context.instance, &physical_device_count, NULL);

		DebugLogAssert(log_channel, physical_device_count > 0, "Failed to find GPUs with Vulkan support.");

		DebugLogD(log_channel, "Found %u physical devices.", physical_device_count);
		
		VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

		VkPhysicalDevice *physical_devices = ArenaPushArray(scratch.arena, VkPhysicalDevice, physical_device_count);

		vkEnumeratePhysicalDevices(context.instance, &physical_device_count, physical_devices);

		u32 best_usability = 0;
		
		for (u32 i = 0; i < physical_device_count; i++)
		{
			VkPhysicalDevice curr_physical_device = physical_devices[i];
			
			vkGetPhysicalDeviceProperties2(curr_physical_device, &properties);
	
			DebugLogD(log_channel,
					  "Querying physical device: %s (%d)",
					  properties.properties.deviceName,
					  properties.properties.deviceID);

			b32 has_essentials = true;

			G_Capabilities detected_caps = {0};
			G_ResolvedCapabilities resolved_caps = {0};
			VkPhysicalDeviceFeatures2 features = {0};
	
			// swapchain
			{
				G_SwapchainSupportDetails details = G_ContextQuerySwapchainSupport(scratch.arena, curr_physical_device, context.surface);

				if ((details.surface_format_count <= 0) ||
					(details.present_mode_count <= 0))
				{
					has_essentials = false;
				}					
			}

			// capabilities
			{
				detected_caps = G_CapabilitiesQuery(curr_physical_device, &features, log_channel);
				resolved_caps = G_CapabilitiesResolve(scratch.arena, curr_physical_device, detected_caps, requirements, log_channel);

				if (!resolved_caps.meets_requirements)
				{
					has_essentials = false;
				}
			}

			u32 current_usability = G_CapabilitiesScore(resolved_caps.enabled, requirements);
			
			if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				current_usability += 4;
			}
			else if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				current_usability += 1;
			}
			
			if (current_usability > best_usability && has_essentials)
			{
				context.physical_device = curr_physical_device;
				context.physical_device_properties = properties;
				context.physical_device_features = features;
				
				context.capabilities = resolved_caps;
				
				best_usability = current_usability;
			}
		}

		DebugLogAssert(log_channel,
					   context.physical_device,
					   "Unable to find a suitable physical device :(");

		DebugLogD(log_channel,
				  "Selected physical device: %s (%d) :)",
				  context.physical_device_properties.properties.deviceName,
				  context.physical_device_properties.properties.deviceID);
	}

	context.max_msaa_samples = G_ContextFindGraphicsMaxUsableSampleCount(context.physical_device_properties, log_channel);
	context.max_push_constants_size = context.physical_device_properties.properties.limits.maxPushConstantsSize;
	
	context.depth_format = G_ContextFindGraphicsDepthFormat(context.physical_device, log_channel);

	u32 queue_family_count = 0;
	
	vkGetPhysicalDeviceQueueFamilyProperties(context.physical_device, &queue_family_count, 0);

	if (queue_family_count <= 0)
		DebugLogD(log_channel, "Failed to find any queue families.");

	VkQueueFamilyProperties *queue_families = ArenaPushArray(scratch.arena, VkQueueFamilyProperties, queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(context.physical_device, &queue_family_count, queue_families);

	for (u32 i = 0; i < queue_family_count; i++)
	{
		if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			VkBool32 present_support = VK_FALSE;

			vkGetPhysicalDeviceSurfaceSupportKHR(context.physical_device, i,
												 context.surface,
												 &present_support);

			if (present_support)
			{
				context.graphics_queue.family_index = i;
				break;
			}

			continue;
		}
	}

	float queue_priority = 1.f;

	VkDeviceQueueCreateInfo graphics_queue_create_info = {0};
	graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphics_queue_create_info.queueFamilyIndex = context.graphics_queue.family_index;
	graphics_queue_create_info.queueCount = 1;
	graphics_queue_create_info.pQueuePriorities = &queue_priority;

	//context.physical_device_features.features.robustBufferAccess = VK_TRUE;
	
	VkPhysicalDeviceVulkan11Features vulkan11_features = {0};
	vulkan11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	vulkan11_features.shaderDrawParameters = VK_TRUE;
	vulkan11_features.multiview = VK_TRUE;

	VkPhysicalDeviceVulkan12Features vulkan12_features = {0};
	vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12_features.runtimeDescriptorArray = VK_TRUE;
	vulkan12_features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	vulkan12_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	vulkan12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	vulkan12_features.descriptorBindingPartiallyBound = VK_TRUE;
	vulkan12_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
	vulkan12_features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	vulkan12_features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	vulkan12_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	vulkan12_features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
	vulkan12_features.bufferDeviceAddress = VK_TRUE;
	vulkan12_features.scalarBlockLayout = VK_TRUE;
	vulkan12_features.timelineSemaphore = VK_TRUE;
	vulkan12_features.drawIndirectCount = VK_TRUE;
	vulkan12_features.pNext = &vulkan11_features;

	VkPhysicalDeviceVulkan13Features vulkan13_features = {0};
	vulkan13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13_features.dynamicRendering = VK_TRUE;
	vulkan13_features.synchronization2 = VK_TRUE;
	vulkan13_features.pNext = &vulkan12_features;

	/*
	VkPhysicalDeviceVulkan14Features vulkan14_features = {0};
	vulkan14_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
	vulkan14_features.maintenance5 = VK_TRUE;
	vulkan14_features.pNext = &vulkan13_features;
	*/
	
	VkPhysicalDeviceMaintenance5FeaturesKHR mt5_features = {0};
	mt5_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR;
	mt5_features.maintenance5 = VK_TRUE;
	mt5_features.pNext = &vulkan13_features;

	VkPhysicalDeviceMaintenance6FeaturesKHR mt6_features = {0};
	mt6_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES_KHR;
	mt6_features.maintenance6 = VK_TRUE;
	mt6_features.pNext = &mt5_features;

	void *feature_chain_tail = &mt6_features;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_features = {0};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_struct_features = {0};
	VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {0};
	
	if (context.capabilities.enabled.set[G_CapabilityType_RayTracingPipeline] &&
		context.capabilities.enabled.set[G_CapabilityType_AccelerationStructure] &&
		context.capabilities.enabled.set[G_CapabilityType_RayQuery])
	{
		rt_pipeline_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		rt_pipeline_features.rayTracingPipeline = VK_TRUE;
		rt_pipeline_features.pNext = feature_chain_tail;

		accel_struct_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		accel_struct_features.accelerationStructure = VK_TRUE;
		accel_struct_features.pNext = &rt_pipeline_features;

		ray_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
		ray_query_features.rayQuery = VK_TRUE;
		ray_query_features.pNext = &accel_struct_features;

		feature_chain_tail = &ray_query_features;
	}

	VkPhysicalDeviceFeatures enabled_base_features = {0};
	enabled_base_features.samplerAnisotropy = context.capabilities.enabled.set[G_CapabilityType_SamplerAnisotropy];
	enabled_base_features.sampleRateShading = context.capabilities.enabled.set[G_CapabilityType_SampleRateShading];
	enabled_base_features.shaderInt64 = context.capabilities.enabled.set[G_CapabilityType_ShaderI64];
	
	VkDeviceCreateInfo device_create_info = {0};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = NULL;
	device_create_info.enabledExtensionCount = context.capabilities.extension_count;
	device_create_info.ppEnabledExtensionNames = context.capabilities.extension_names;
	device_create_info.pEnabledFeatures = &enabled_base_features;
	device_create_info.pNext = feature_chain_tail;
	
	G_VK_CHECK(vkCreateDevice(context.physical_device,
							  &device_create_info, NULL,
							  &context.device),
			   "Failed to create logical device.");

	DebugLogD(log_channel, "Created logical device.");
	
	vkGetDeviceQueue(context.device,
					 context.graphics_queue.family_index, 0,
					 &context.graphics_queue.vk_handle);

	u32 instance_version = 0;
	VkResult result = vkEnumerateInstanceVersion(&instance_version);

	if (result == VK_SUCCESS)
	{
		u32 device_version = context.physical_device_properties.properties.apiVersion;
		
		u32 i_major = VK_API_VERSION_MAJOR(instance_version);
		u32 i_minor = VK_API_VERSION_MINOR(instance_version);
		i32 i_patch = VK_API_VERSION_PATCH(instance_version);

		u32 d_major = VK_API_VERSION_MAJOR(device_version);
		u32 d_minor = VK_API_VERSION_MINOR(device_version);
		u32 d_patch = VK_API_VERSION_PATCH(device_version);
		
		DebugLogD(log_channel,
				  "Instance Version: %d.%d.%d, Device Version: %d.%d.%d",
				  i_major, i_minor, i_patch,
				  d_major, d_minor, d_patch);
	}
	else
	{
		DebugLogE(log_channel, "Failed to retrieve Vulkan version, something's gone wrong probably.");
	}

	volkLoadDevice(context.device);

	VmaVulkanFunctions vulkan_functions = {0};
	vulkan_functions.vkAllocateMemory                    = vkAllocateMemory;
	vulkan_functions.vkBindBufferMemory                  = vkBindBufferMemory;
	vulkan_functions.vkBindImageMemory                   = vkBindImageMemory;
	vulkan_functions.vkCreateBuffer                      = vkCreateBuffer;
	vulkan_functions.vkCreateImage                       = vkCreateImage;
	vulkan_functions.vkDestroyBuffer                     = vkDestroyBuffer;
	vulkan_functions.vkDestroyImage                      = vkDestroyImage;
	vulkan_functions.vkFlushMappedMemoryRanges           = vkFlushMappedMemoryRanges;
	vulkan_functions.vkFreeMemory                        = vkFreeMemory;
	vulkan_functions.vkGetBufferMemoryRequirements       = vkGetBufferMemoryRequirements;
	vulkan_functions.vkGetImageMemoryRequirements        = vkGetImageMemoryRequirements;
	vulkan_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vulkan_functions.vkGetPhysicalDeviceProperties       = vkGetPhysicalDeviceProperties;
	vulkan_functions.vkInvalidateMappedMemoryRanges      = vkInvalidateMappedMemoryRanges;
	vulkan_functions.vkMapMemory                         = vkMapMemory;
	vulkan_functions.vkUnmapMemory                       = vkUnmapMemory;
	vulkan_functions.vkCmdCopyBuffer                     = vkCmdCopyBuffer;

	VmaAllocatorCreateInfo allocator_create_info = {0};
	allocator_create_info.physicalDevice = context.physical_device;
	allocator_create_info.device = context.device;
	allocator_create_info.instance = context.instance;
	allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	//vmaImportVulkanFunctionsFromVolk(&allocator_create_info, &vulkan_functions);

	allocator_create_info.pVulkanFunctions = &vulkan_functions;

	G_VK_CHECK(vmaCreateAllocator(&allocator_create_info,
								  &context.vma_allocator),
			   "Failed to create Vulkan Memory Allocator.");
	
	DebugLogD(log_channel, "Created Vulkan Memory Allocator.");

	context.swapchain_details = G_ContextQuerySwapchainSupport(arena, context.physical_device, context.surface);

	VkPipelineCacheCreateInfo pipeline_cache_create_info = {0};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = NULL;
	pipeline_cache_create_info.flags = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = NULL;

	G_VK_CHECK(vkCreatePipelineCache(context.device,
									 &pipeline_cache_create_info, NULL,
									 &context.pipeline_process_cache),
			   "Failed to process pipeline cache.");

	ScratchRelease(&scratch);

	return context;
}

internal void G_ContextDestroy(G_Context *context)
{
	vkDestroyPipelineCache(context->device, context->pipeline_process_cache, NULL);
	osapi->VulkanSurfaceDestroy(context->instance, context->surface);
	vmaDestroyAllocator(context->vma_allocator);
	vkDestroyDebugUtilsMessengerEXT(context->instance, context->debug_messenger, NULL);
	vkDestroyDevice(context->device, NULL);
}
