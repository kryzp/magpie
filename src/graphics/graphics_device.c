
static G_Device *g_device = NULL;

static const char *g_device_vk_validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};

static const G_FeatureDef g_device_feature_defs[] = {
	[G_FeatureType_RayTracing] = {
		.name = "raytracing",
		.capability_count = 3,
		.capabilities[0] = G_CapabilityType_RayTracingPipeline,
		.capabilities[1] = G_CapabilityType_AccelerationStructure,
		.capabilities[2] = G_CapabilityType_RayQuery
	}
};

internal VkFormat G_DeviceFindGraphicsSupportedFormat(VkPhysicalDevice physical_device,
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

internal VkFormat G_DeviceFindGraphicsDepthFormat(VkPhysicalDevice physical_device, LOG_Channel log_channel)
{
	static const VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	return G_DeviceFindGraphicsSupportedFormat(physical_device,
											   VK_IMAGE_TILING_OPTIMAL,
											   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
											   ArraySize(candidates), candidates,
											   log_channel);
}

internal VkSampleCountFlagBits G_DeviceFindGraphicsMaxUsableSampleCount(VkPhysicalDeviceProperties2 properties, LOG_Channel log_channel)
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

internal const char * const *G_DeviceGetInstanceExtensions(Arena *arena, u32 *extension_count, LOG_Channel log_channel)
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

internal b32 G_DeviceCheckForValidationLayerSupport(void)
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);

	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkLayerProperties *available_layers = ArenaPushArray(scratch.arena, VkLayerProperties, layer_count);
	
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	b32 result = true;

	for (u32 i = 0; i < ArraySize(g_device_vk_validation_layers); i++)
	{
		b32 has_layer = false;
		const char *layer_name_0 = g_device_vk_validation_layers[i];

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

internal G_SwapchainSupportDetails G_DeviceQuerySwapchainSupport(Arena *arena,
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

internal VkResult G_DeviceCreateDeviceDebugUtilsMessengerExt(VkInstance instance,
															 VkDebugUtilsMessengerCreateInfoEXT *debug_info,
															 const VkAllocationCallbacks *allocator,
															 VkDebugUtilsMessengerEXT *messenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn)
		return fn(instance, debug_info, allocator, messenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

internal VkSurfaceFormatKHR G_DeviceChooseSwapchainSurfaceFormat(LOG_Channel channel,
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


internal VkPresentModeKHR G_DeviceChooseSwapchainPresentMode(u32 available_present_mode_count,
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

internal VkExtent2D G_DeviceChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR *capabilities)
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

internal VKAPI_ATTR VkBool32 VKAPI_CALL G_DeviceVulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
																	VkDebugUtilsMessageTypeFlagsEXT message_type,
																	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
																	void *ctx)
{
	LOG_Channel ch = g_device->log_channel;

	switch (message_type)
	{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			ch = g_device->log_channel_general;
			break;
			
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			ch = g_device->log_channel_validation;
			break;
			
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			ch = g_device->log_channel_performance;
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

internal void G_DeviceInitAndSelect(G_Device *device, Arena *arena, LOG_Channel log_channel,
									G_FeatureRequest *requested_features, u32 feature_count)
{
	ScratchArena scratch = ScratchBegin(&arena, 1);
		
	device->permanent_arena = arena;

	device->log_channel = log_channel;
	device->log_channel_general = osapi->LogChannelOpenFrom(log_channel, String8Lit("GENERAL"));
	device->log_channel_validation = osapi->LogChannelOpenFrom(log_channel, String8Lit("VALIDATION"));
	device->log_channel_performance = osapi->LogChannelOpenFrom(log_channel, String8Lit("PERFORMANCE"));
	device->log_channel_cmd_buffer = osapi->LogChannelOpenFrom(log_channel, String8Lit("CMD"));
	
	for (u32 i = 0; i < ArraySize(device->frames_in_flight); i++)
		device->frames_in_flight[i].arena = ArenaAlloc(Megabytes(64));

	device->current_frame_in_flight_index = 0;

	G_ResourceListInit(&device->pipeline_layouts);
	G_ResourceListInit(&device->pipelines);
	G_ResourceListInit(&device->textures);
	G_ResourceListInit(&device->texture_views);
	G_ResourceListInit(&device->buffers);
	G_ResourceListInit(&device->samplers);
	G_ResourceListInit(&device->shaders);
	G_ResourceListInit(&device->accel_structures);

	G_DeviceSelectContext(device);
	
	static const char *base_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
		VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
		VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
	};

	G_Feature *features = ArenaPushArray(scratch.arena, G_Feature, feature_count);

	for (u32 i = 0; i < feature_count; i++)
	{
		features[i].def = g_device_feature_defs[requested_features[i].type];
		features[i].type = requested_features[i].type;
		features[i].tier = requested_features[i].tier;
	}
	
	G_Requirements requirements = {0};
	requirements.base_extension_count = ArraySize(base_extensions);
	requirements.base_extensions = base_extensions;
	requirements.feature_count = feature_count;
	requirements.features = features;

	{
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

		instance_create_info.ppEnabledExtensionNames = G_DeviceGetInstanceExtensions(scratch.arena, &instance_create_info.enabledExtensionCount, log_channel);

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

		debug_create_info.pfnUserCallback = G_DeviceVulkanDebugCallback;
		debug_create_info.pUserData = NULL;
	
		device->has_validation_layers = G_DeviceCheckForValidationLayerSupport();

		if (device->has_validation_layers)
		{
			DebugLogD(log_channel, "Validation layer support verified.");

			instance_create_info.enabledLayerCount = ArraySize(g_device_vk_validation_layers);
			instance_create_info.ppEnabledLayerNames = g_device_vk_validation_layers;
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
	
		G_VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &device->vk_instance),
				   "Failed to create instance.");

		volkLoadInstance(device->vk_instance);

		if (device->has_validation_layers)
		{
			G_VK_CHECK(G_DeviceCreateDeviceDebugUtilsMessengerExt(device->vk_instance,
																  &debug_create_info, NULL,
																  &device->debug_messenger),
					   "Failed to create debug messenger.");
		}

		if (!osapi->VulkanSurfaceCreate(device->vk_instance, &device->vk_surface))
			DebugLogB(log_channel, "Failed to create surface.");

		// Enumerate physical_resource devices.
		{
			u32 physical_device_count = 0;
			vkEnumeratePhysicalDevices(device->vk_instance, &physical_device_count, NULL);

			DebugLogAssert(log_channel, physical_device_count > 0, "Failed to find GPUs with Vulkan support.");

			DebugLogD(log_channel, "Found %u physical devices.", physical_device_count);
		
			VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

			VkPhysicalDevice *physical_devices = ArenaPushArray(scratch.arena, VkPhysicalDevice, physical_device_count);

			vkEnumeratePhysicalDevices(device->vk_instance, &physical_device_count, physical_devices);

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
				G_ResolvedFeatures resolved_features = {0};
				VkPhysicalDeviceFeatures2 features = {0};
	
				// swapchain
				{
					G_SwapchainSupportDetails details = G_DeviceQuerySwapchainSupport(scratch.arena, curr_physical_device, device->vk_surface);

					if ((details.surface_format_count <= 0) ||
						(details.present_mode_count <= 0))
					{
						has_essentials = false;
					}					
				}

				// capabilities
				{
					detected_caps = G_CapabilitiesQuery(curr_physical_device, &features, log_channel);
					resolved_features = G_FeaturesResolve(scratch.arena, curr_physical_device, detected_caps, &requirements, log_channel);

					if (!resolved_features.meets_requirements)
					{
						has_essentials = false;
					}
				}

				u32 current_usability = G_FeaturesScore(resolved_features.enabled, &requirements);
			
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
					device->vk_physical_device = curr_physical_device;
					device->vk_physical_device_properties = properties;
					device->vk_physical_device_features = features;
				
					device->features = resolved_features;
				
					best_usability = current_usability;
				}
			}

			DebugLogAssert(log_channel,
						   device->vk_physical_device,
						   "Unable to find a suitable physical device :(");

			DebugLogD(log_channel,
					  "Selected physical device: %s (%d) :)",
					  device->vk_physical_device_properties.properties.deviceName,
					  device->vk_physical_device_properties.properties.deviceID);
		}

		device->max_msaa_samples = G_DeviceFindGraphicsMaxUsableSampleCount(device->vk_physical_device_properties, log_channel);
		device->max_push_constants_size = device->vk_physical_device_properties.properties.limits.maxPushConstantsSize;
	
		device->depth_format = G_DeviceFindGraphicsDepthFormat(device->vk_physical_device, log_channel);

		u32 queue_family_count = 0;
	
		vkGetPhysicalDeviceQueueFamilyProperties(device->vk_physical_device, &queue_family_count, 0);

		if (queue_family_count <= 0)
			DebugLogD(log_channel, "Failed to find any queue families.");

		VkQueueFamilyProperties *queue_families = ArenaPushArray(scratch.arena, VkQueueFamilyProperties, queue_family_count);

		vkGetPhysicalDeviceQueueFamilyProperties(device->vk_physical_device, &queue_family_count, queue_families);

		for (u32 i = 0; i < queue_family_count; i++)
		{
			if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				VkBool32 present_support = VK_FALSE;

				vkGetPhysicalDeviceSurfaceSupportKHR(device->vk_physical_device, i,
													 device->vk_surface,
													 &present_support);

				if (present_support)
				{
					device->graphics_queue.family_index = i;
					break;
				}

				continue;
			}
		}

		float queue_priority = 1.f;

		VkDeviceQueueCreateInfo graphics_queue_create_info = {0};
		graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		graphics_queue_create_info.queueFamilyIndex = device->graphics_queue.family_index;
		graphics_queue_create_info.queueCount = 1;
		graphics_queue_create_info.pQueuePriorities = &queue_priority;

		VkPhysicalDeviceFeatures enabled_base_features = {0};
		enabled_base_features.samplerAnisotropy = VK_TRUE;
		enabled_base_features.sampleRateShading = VK_TRUE;
		enabled_base_features.shaderInt64 = VK_TRUE;
		enabled_base_features.multiDrawIndirect = VK_TRUE;
		//enabled_base_features.robustBufferAccess = VK_TRUE;
	
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
		
		if (G_DeviceFeatureEnabled(G_FeatureType_RayTracing))
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
	
		VkDeviceCreateInfo device_create_info = {0};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.queueCreateInfoCount = 1;
		device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
		device_create_info.enabledLayerCount = 0;
		device_create_info.ppEnabledLayerNames = NULL;
		device_create_info.enabledExtensionCount = device->features.extension_count;
		device_create_info.ppEnabledExtensionNames = device->features.extension_names;
		device_create_info.pEnabledFeatures = &enabled_base_features;
		device_create_info.pNext = feature_chain_tail;
	
		G_VK_CHECK(vkCreateDevice(device->vk_physical_device,
								  &device_create_info, NULL,
								  &device->vk_device),
				   "Failed to create logical device.");

		DebugLogD(log_channel, "Created logical device.");
	
		vkGetDeviceQueue(device->vk_device,
						 device->graphics_queue.family_index, 0,
						 &device->graphics_queue.vk_handle);

		u32 instance_version = 0;
		VkResult result = vkEnumerateInstanceVersion(&instance_version);

		if (result == VK_SUCCESS)
		{
			u32 device_version = device->vk_physical_device_properties.properties.apiVersion;
		
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

		volkLoadDevice(device->vk_device);

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
		allocator_create_info.physicalDevice = device->vk_physical_device;
		allocator_create_info.device = device->vk_device;
		allocator_create_info.instance = device->vk_instance;
		allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		//vmaImportVulkanFunctionsFromVolk(&allocator_create_info, &vulkan_functions);

		allocator_create_info.pVulkanFunctions = &vulkan_functions;

		G_VK_CHECK(vmaCreateAllocator(&allocator_create_info,
									  &device->vma_allocator),
				   "Failed to create Vulkan Memory Allocator.");
	
		DebugLogD(log_channel, "Created Vulkan Memory Allocator.");

		device->swapchain_details = G_DeviceQuerySwapchainSupport(arena, device->vk_physical_device, device->vk_surface);

		VkPipelineCacheCreateInfo pipeline_cache_create_info = {0};
		pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		pipeline_cache_create_info.pNext = NULL;
		pipeline_cache_create_info.flags = 0;
		pipeline_cache_create_info.initialDataSize = 0;
		pipeline_cache_create_info.pInitialData = NULL;

		G_VK_CHECK(vkCreatePipelineCache(device->vk_device,
										 &pipeline_cache_create_info, NULL,
										 &device->pipeline_process_cache),
				   "Failed to process pipeline cache.");
	}

	G_DeviceCreateSyncResources();
	G_DeviceCreateBindless();
	G_DeviceCreateImGui();

	DebugLogI(g_device->log_channel, "Initialized.");

	ScratchRelease(&scratch);
}

internal void G_DeviceDestroy(void)
{
	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
	{
		G_DeviceFrameInFlight *frame = &g_device->frames_in_flight[i];
		G_DeviceWaitUntil(frame->completion_point);
		G_DeviceFlushInFlightFrame(frame);
		ArenaRelease(&frame->arena);
	}
	
	// pipeline layouts, pipelines and texture views are special and cached
	// internally without any API to destroy them so we
	// have to destroy them here.

	for (G_ResourceListNode *node = g_device->pipeline_layouts.first_sentinel.next;
		 node != &g_device->pipeline_layouts.first_sentinel;
		 node = node->next)
	{
		vkDestroyPipelineLayout(g_device->vk_device, node->resource.pipeline_layout, NULL);
	}

	for (G_ResourceListNode *node = g_device->pipelines.first_sentinel.next;
		 node != &g_device->pipelines.first_sentinel;
		 node = node->next)
	{
		vkDestroyPipeline(g_device->vk_device, node->resource.pipeline, NULL);
	}
	
	for (G_ResourceListNode *node = g_device->texture_views.first_sentinel.next;
		 node != &g_device->texture_views.first_sentinel;
		 node = node->next)
	{
		vkDestroyImageView(g_device->vk_device, node->resource.texture_view.vk_handle, NULL);
	}
	
	G_DeviceDestroyImGui();
	G_DeviceDestroyBindless();
	G_DeviceDestroySyncResources();

	vkDestroyPipelineCache(g_device->vk_device, g_device->pipeline_process_cache, NULL);
	osapi->VulkanSurfaceDestroy(g_device->vk_instance, g_device->vk_surface);
	vmaDestroyAllocator(g_device->vma_allocator);
	vkDestroyDebugUtilsMessengerEXT(g_device->vk_instance, g_device->debug_messenger, NULL);
	vkDestroyDevice(g_device->vk_device, NULL);

	DebugLogI(g_device->log_channel, "Destroyed.");

	g_device = NULL;
}

internal void G_DeviceSelectContext(G_Device *device)
{
	g_device = device;
}

internal G_Device *G_DeviceGetSelected(void)
{
	return g_device;
}

internal VkFormat G_DeviceDepthFormat(void)
{
	return g_device->depth_format;
}

internal u32 G_DeviceFrameInFlightIndex(void)
{
	return g_device->current_frame_in_flight_index;
}

internal b32 G_DeviceFeatureEnabled(G_FeatureType type)
{
	return g_device->features.enabled.set[type];
}

internal void G_DeviceFlushInFlightFrame(G_DeviceFrameInFlight *frame)
{
	for (G_DestroyedImage *img = frame->destroyed_image_head; img; img = img->next)
	{
		G_ResourceListReturn(&g_device->textures, img->key);
		vmaDestroyImage(g_device->vma_allocator, img->image, img->allocation);
	}

	for (G_DestroyedBuffer *b = frame->destroyed_buffer_head; b; b = b->next)
	{
		G_ResourceListReturn(&g_device->buffers, b->key);
		vmaDestroyBuffer(g_device->vma_allocator, b->buffer, b->allocation);
	}
	
	for (G_DestroyedSampler *s = frame->destroyed_sampler_head; s; s = s->next)
	{
		G_ResourceListReturn(&g_device->samplers, s->key);
		vkDestroySampler(g_device->vk_device, s->sampler, NULL);
		G_BindlessFreeSampler(&g_device->bindless, s->bindless);
	}
	
	for (G_DestroyedShaderProgram *sh = frame->destroyed_shader_head; sh; sh = sh->next)
	{
		G_ResourceListReturn(&g_device->shaders, sh->key);
	}
	
	for (G_DestroyedAccelStruct *a = frame->destroyed_as_head; a; a = a->next)
	{
		G_ResourceListReturn(&g_device->accel_structures, a->key);
		vkDestroyAccelerationStructureKHR(g_device->vk_device, a->handle, NULL);
	}
	
	frame->destroyed_sampler_head = NULL;
	frame->destroyed_image_head = NULL;
	frame->destroyed_buffer_head = NULL;

	ArenaReset(&frame->arena);
}

// TODO: BeginFrame / EndFrame should be moved into the swapchain.

internal G_CmdBuffer G_DeviceBeginFrame(G_Swapchain *swapchain)
{
	G_DeviceFrameInFlight *frame_in_flight = &g_device->frames_in_flight[g_device->current_frame_in_flight_index];

	G_DeviceWaitUntil(frame_in_flight->completion_point);
	G_DeviceFlushInFlightFrame(frame_in_flight);

	VkAcquireNextImageInfoKHR acquire_next_image_info = {0};
	acquire_next_image_info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquire_next_image_info.swapchain = swapchain->vk_handle;
	acquire_next_image_info.timeout = UINT64_MAX;
	acquire_next_image_info.semaphore = frame_in_flight->image_available_semaphore;
	acquire_next_image_info.fence = VK_NULL_HANDLE;
	acquire_next_image_info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(g_device->vk_device, &acquire_next_image_info, &swapchain->current_frame_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		DebugLogB(g_device->log_channel, "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		DebugLogB(g_device->log_channel, "Failed to acquire next image in swapchain.");

	G_DeviceCmdPoolPurge(&frame_in_flight->command_pool, frame_in_flight->completion_point.frame);

	G_CmdBuffer cmd = G_DeviceCmdPoolAcquire(&frame_in_flight->command_pool);
	G_CmdBegin(&cmd);

	return cmd;
}

internal void G_DeviceEndFrame(const G_Swapchain *swapchain, const G_CmdBuffer *cmd)
{
	G_DeviceApplyBindlessUpdates();

	G_DeviceFrameInFlight *frame_in_flight = &g_device->frames_in_flight[g_device->current_frame_in_flight_index];
	const G_SwapchainFrame *swapchain_frame = &swapchain->frames[swapchain->current_frame_index];
	
	VkSemaphoreSubmitInfo image_available_semaphore_info = {0};
	image_available_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore_info.semaphore = frame_in_flight->image_available_semaphore;
	image_available_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSemaphoreSubmitInfo render_finished_semaphore_info = {0};
	render_finished_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore_info.semaphore = swapchain_frame->render_finished_semaphore;
	render_finished_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // TODO: Do we need to be waiting on VK_PIPELINE_STAGE_2_ALL_COMMANDS ????

	frame_in_flight->completion_point = G_DeviceSubmitEx(cmd,
														 1, &image_available_semaphore_info,
														 1, &render_finished_semaphore_info);
	
	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = NULL;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain->vk_handle;
	present_info.pImageIndices = &swapchain->current_frame_index;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &swapchain_frame->render_finished_semaphore;

	VkResult result = vkQueuePresentKHR(g_device->graphics_queue.vk_handle, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		DebugLogB(g_device->log_channel, "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		DebugLogB(g_device->log_channel, "Failed to present swapchain image. (%u)", result);

	G_DeviceCmdPoolRelease(&frame_in_flight->command_pool, cmd, frame_in_flight->completion_point.frame);
	
	g_device->current_frame_in_flight_index = (g_device->current_frame_in_flight_index + 1) % G_FRAMES_IN_FLIGHT;
}

internal G_TimelinePoint G_DeviceSubmit(const G_CmdBuffer *cmd)
{
	return G_DeviceSubmitEx(cmd, 0, NULL, 0, NULL);
}

internal G_TimelinePoint G_DeviceSubmitEx(const G_CmdBuffer *cmd,
										  u32 wait_count, const VkSemaphoreSubmitInfo *waits,
										  u32 signal_count, const VkSemaphoreSubmitInfo *signals)
{
	G_CmdEnd(cmd);

	G_TimelinePoint timeline_point = G_SemaphoreSignal(&g_device->graphics_semaphore);

	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkSemaphoreSubmitInfo *all_signals = ArenaPushArray(scratch.arena, VkSemaphoreSubmitInfo, 1 + signal_count);

	VkSemaphoreSubmitInfo timeline_signal_info = {0};
	timeline_signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	timeline_signal_info.semaphore = timeline_point.semaphore;
	timeline_signal_info.value = timeline_point.frame;
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

	G_VK_CHECK(vkQueueSubmit2(g_device->graphics_queue.vk_handle, 1, &submit_info, VK_NULL_HANDLE),
			   "Failed to submit command to queue.");

	ScratchRelease(&scratch);

	return timeline_point;
}

internal G_CmdBuffer G_DeviceSubmitImBegin(void)
{
	vkQueueWaitIdle(g_device->graphics_queue.vk_handle);

	G_CmdBuffer cmd = G_DeviceCmdPoolAcquire(&g_device->frames_in_flight[g_device->current_frame_in_flight_index].command_pool);
	G_CmdBegin(&cmd);

	return cmd;
}

internal void G_DeviceSubmitImEnd(const G_CmdBuffer *cmd)
{
	G_DeviceWaitUntil(G_DeviceSubmit(cmd));
}

// TODO: We really shouldn't need to initialize
//       and de-initialize volk like this. Surely
//       we should load all fpointers into a table
//       which we store in program memory (possible
//       in volk) and just load that table back
//       in after reloading?
//       --> Also, do VMA's function pointers
//           break down here? Or is it okay?

internal void G_DeviceHotLoad(void)
{
	volkInitialize();
	volkLoadInstance(g_device->vk_instance);
	volkLoadDevice(g_device->vk_device);

	G_DeviceWaitIdle();
}

internal void G_DeviceHotUnload(void)
{
	volkFinalize();
}

internal VkQueryPool G_DeviceQueryPoolCreate(u32 query_count, VkQueryType type, VkQueryPipelineStatisticFlags pipeline_stat_flags)
{
	VkQueryPoolCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	create_info.queryCount = query_count;
	create_info.queryType = type;
	create_info.pipelineStatistics = pipeline_stat_flags;
	
	VkQueryPool handle = VK_NULL_HANDLE;

	G_VK_CHECK(vkCreateQueryPool(g_device->vk_device,
								 &create_info,
								 NULL,
								 &handle),
			   "Failed to create query pool.");
	
	return handle;
}

internal void G_DeviceQueryPoolDestroy(VkQueryPool pool)
{
	vkDestroyQueryPool(g_device->vk_device, pool, NULL);
}

internal void G_DeviceWaitIdle(void)
{
	vkDeviceWaitIdle(g_device->vk_device);
}

internal void G_DeviceWaitForFence(VkFence fence)
{
	vkWaitForFences(g_device->vk_device, 1, &fence, VK_TRUE, UINT64_MAX);
}

internal void G_DeviceResetFence(VkFence fence)
{
	vkResetFences(g_device->vk_device, 1, &fence);
}

internal void G_DeviceDestroyFence(VkFence fence)
{
	vkDestroyFence(g_device->vk_device, fence, NULL);
}

internal G_Semaphore G_DeviceSemaphoreCreate(u64 value)
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

	G_VK_CHECK(vkCreateSemaphore(g_device->vk_device,
								 &timeline_semaphore_create_info, NULL,
								 &vk_semaphore),
			   "Failed to create timeline semaphore.");

	G_Semaphore semaphore = {0};
	semaphore.vk_handle = vk_semaphore;
	semaphore.last_submitted_frame = value;

	return semaphore;
}

internal void G_DeviceSemaphoreDestroy(const G_Semaphore *semaphore)
{
	vkDestroySemaphore(g_device->vk_device, semaphore->vk_handle, NULL);
}

internal u64 G_DeviceSemaphoreGPUCounterValue(const G_Semaphore *semaphore)
{
	u64 result = 0;

	vkGetSemaphoreCounterValue(g_device->vk_device,
							   semaphore->vk_handle,
							   &result);

	return result;
}

internal void G_DeviceWaitUntil(G_TimelinePoint point)
{
	if (point.frame == 0)
		return;

	VkSemaphoreWaitInfo wait_info = {0};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores = &point.semaphore;
	wait_info.pValues = &point.frame;

	G_VK_CHECK(vkWaitSemaphores(g_device->vk_device,
								&wait_info, UINT64_MAX),
			   "Failed to wait on timeline semaphore");
}

internal G_Swapchain G_DeviceSwapchainCreate(void)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	const G_SwapchainSupportDetails *details = &g_device->swapchain_details;

	VkSurfaceFormatKHR surface_format = G_DeviceChooseSwapchainSurfaceFormat(g_device->log_channel, details->surface_format_count, details->surface_formats);
	VkPresentModeKHR present_mode = G_DeviceChooseSwapchainPresentMode(details->present_mode_count, details->present_modes, false); // TODO: add option to enable VSYNC
	VkExtent2D extent = G_DeviceChooseSwapchainExtent(&details->capabilities);

	G_Swapchain swapchain = {0};

	swapchain.width = extent.width;
	swapchain.height = extent.height;
	swapchain.format = surface_format.format;
	
	u32 frame_count = details->capabilities.minImageCount + 1;

	if (frame_count > details->capabilities.maxImageCount && details->capabilities.maxImageCount > 0)
		frame_count = details->capabilities.maxImageCount;

	DebugLogD(g_device->log_channel, "Swapchain:");
	DebugLogD(g_device->log_channel, " size: %u x %u", extent.width, extent.height);
	DebugLogD(g_device->log_channel, " format: %u", surface_format.format);
	DebugLogD(g_device->log_channel, " frame count: %u", frame_count);
	
	const VkImageUsageFlags swapchain_texture_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkSwapchainCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = g_device->vk_surface;
	create_info.minImageCount = frame_count;
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

	G_VK_CHECK(vkCreateSwapchainKHR(g_device->vk_device,
									&create_info, NULL,
									&swapchain.vk_handle),
			   "Failed to create swapchain.");

	vkGetSwapchainImagesKHR(g_device->vk_device, swapchain.vk_handle, &frame_count, NULL);

	DebugLogAssert(g_device->log_channel, frame_count > 0, "Failed to find any images in swapchain.");

	VkImage *vk_images = ArenaPushArray(scratch.arena, VkImage, frame_count);

	vkGetSwapchainImagesKHR(g_device->vk_device, swapchain.vk_handle, &frame_count, vk_images);

	swapchain.frame_count = frame_count;
	swapchain.frames = ArenaPushArray(g_device->permanent_arena, G_SwapchainFrame, frame_count);

	for (u32 i = 0; i < frame_count; i++)
	{
		G_SwapchainFrame *frame = &swapchain.frames[i];

		// texture
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

			G_Resource resource = {0};
			resource.texture = texture;
			
			frame->texture_key = G_ResourceListPushAuto(&g_device->textures, g_device->permanent_arena, &resource);
		}

		// view
		{
			frame->texture_view.type = VK_IMAGE_VIEW_TYPE_2D;
			frame->texture_view.range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
			frame->texture_view.range.base_mip = 0;
			frame->texture_view.range.mips = 1;
			frame->texture_view.range.base_layer = 0;
			frame->texture_view.range.layers = 1;

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

			G_VK_CHECK(vkCreateImageView(g_device->vk_device,
										 &view_create_info, NULL,
										 &frame->texture_view.vk_handle),
					   "Failed to create texture image view.");
		}

		// semaphore
		{
			VkSemaphoreCreateInfo binary_semaphore_create_info = {0};
			binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			binary_semaphore_create_info.flags = 0;
			binary_semaphore_create_info.pNext = NULL;

			G_VK_CHECK(vkCreateSemaphore(g_device->vk_device,
										 &binary_semaphore_create_info, NULL,
										 &frame->render_finished_semaphore),
					   "Failed to create render finished semaphore.");
		}
	}

	DebugLogD(g_device->log_channel, "Swapchain created.");

	ScratchRelease(&scratch);

	return swapchain;
}

internal void G_DeviceSwapchainDestroy(const G_Swapchain *swapchain)
{
	for (u32 i = 0; i < swapchain->frame_count; i++)
	{
		const G_SwapchainFrame *frame = &swapchain->frames[i];
		vkDestroyImageView(g_device->vk_device, frame->texture_view.vk_handle, NULL);
		vkDestroySemaphore(g_device->vk_device, frame->render_finished_semaphore, NULL);
	}

	vkDestroySwapchainKHR(g_device->vk_device, swapchain->vk_handle, NULL);
}

internal G_CmdPool G_DeviceCmdPoolCreate(u32 family_index)
{
	VkCommandPoolCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;

	G_CmdPool pool = {0};

	G_VK_CHECK(vkCreateCommandPool(g_device->vk_device,
								   &create_info, NULL,
								   &pool.vk_handle),
			   "Failed to create command pool.");

	pool.acquire_count = 0;

	pool.release_front = 0;
	pool.release_count = 0;

	return pool;
}

internal void G_DeviceCmdPoolDestroy(const G_CmdPool *pool)
{
	vkDestroyCommandPool(g_device->vk_device, pool->vk_handle, NULL);
}

internal G_CmdBuffer G_DeviceCmdPoolAcquire(G_CmdPool *pool)
{
	VkCommandBuffer cb = VK_NULL_HANDLE;
	
	if (pool->acquire_count > 0)
	{
		cb = pool->acquire_stack[--pool->acquire_count];
		
		vkResetCommandBuffer(cb, 0);

		//DebugLogT(g_device->log_channel, "Reused");
	}
	else
	{
		VkCommandBufferAllocateInfo alloc_info = {0};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;
		alloc_info.commandPool = pool->vk_handle;
		
		G_VK_CHECK(vkAllocateCommandBuffers(g_device->vk_device,
											&alloc_info,
											&cb),
				   "Failed to allocate command pool command buffers.");
		
		//DebugLogT(g_device->log_channel, "Allocation");
	}

	return G_CmdInit(cb);
}

internal void G_DeviceCmdPoolRelease(G_CmdPool *pool, const G_CmdBuffer *cmd, u64 fence_value)
{
	DebugLogAssert(g_device->log_channel,
				   pool->release_count < ArraySize(pool->release_queue),
				   "Command pool release queue is full.");
	
	u32 slot = (pool->release_front + pool->release_count) % ArraySize(pool->release_queue);

	G_CmdPoolReleasedBuffer *released = &pool->release_queue[slot];
	
	released->vk_handle = cmd->vk_handle;
	released->fence_value = fence_value;

	pool->release_count++;
}

internal void G_DeviceCmdPoolPurge(G_CmdPool *pool, u64 fence_value)
{
	while (pool->release_count > 0)
	{
		G_CmdPoolReleasedBuffer *released = &pool->release_queue[pool->release_front];

		if (released->fence_value > fence_value)
			break;

		DebugLogAssert(g_device->log_channel,
					   pool->acquire_count < ArraySize(pool->acquire_stack),
					   "Command pool acquire stack is full.");

		pool->acquire_stack[pool->acquire_count++] = released->vk_handle;
		
		pool->release_front = (pool->release_front + 1) % ArraySize(pool->release_queue);
		
		pool->release_count--;
	}
}

internal G_ResourceKey G_DevicePipelineLayoutFetch(G_ResourceKey program)
{
	G_ResourceKey pipeline_layout_key = program; // 1:1, only dependent on the program
	
	if (G_DevicePipelineLayoutFromKey(program))
		return pipeline_layout_key;
	
	const G_ShaderProgram *gfx_program = G_DeviceShaderProgramFromKey(program);
	
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
	create_info.pSetLayouts = &g_device->bindless.layout;

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

	G_VK_CHECK(vkCreatePipelineLayout(g_device->vk_device,
									  &create_info, NULL,
									  &layout),
			   "Failed to create pipeline layout.");

	G_Resource resource = {0};
	resource.pipeline_layout = layout;
	
	return G_ResourceListPush(&g_device->pipeline_layouts, g_device->permanent_arena, &resource, pipeline_layout_key);
}

internal VkPipelineLayout G_DevicePipelineLayoutFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->pipeline_layouts, key);
	return resource ? resource->pipeline_layout : VK_NULL_HANDLE;
}

internal G_PipelineSt G_DeviceFetchGraphicsPipeline(const G_GraphicsPipelineDef *def)
{
	u64 hashed_key_value = HashBytesGeneric(def, sizeof(*def));
	G_ResourceKey hashed_key = { hashed_key_value };

	G_ResourceKey layout_key = G_DevicePipelineLayoutFetch(def->program);
	
	if (G_DevicePipelineFromKey(hashed_key))
	{
		G_PipelineSt st = {0};
		st.pipeline = hashed_key;
		st.layout = layout_key;
		st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
		
		return st;
	}
	
	VkPipelineLayout layout = G_DevicePipelineLayoutFromKey(layout_key);
	
	G_ShaderProgram *program = G_DeviceShaderProgramFromKey(def->program);

	DebugLogAssert(g_device->log_channel, !G_ShaderProgramIsCompute(program), "Shader program must not be a compute shader!");

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

		bs->blendEnable = def->blend_state.enabled;

		bs->srcColorBlendFactor = def->blend_state.colour.src;
		bs->dstColorBlendFactor = def->blend_state.colour.dst;
		bs->colorBlendOp = def->blend_state.colour.op;

		bs->srcAlphaBlendFactor = def->blend_state.alpha.src;
		bs->dstAlphaBlendFactor = def->blend_state.alpha.dst;
		bs->alphaBlendOp = def->blend_state.alpha.op;

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
		? G_DeviceDepthFormat()
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

	G_VK_CHECK(vkCreateGraphicsPipelines(g_device->vk_device,
										 g_device->pipeline_process_cache,
										 1, &graphics_pipeline_create_info,
										 NULL, &pipeline),
			   "Failed to create graphics pipeline.");

	G_Resource resource = {0};
	resource.pipeline = pipeline;
	
	G_PipelineSt st = {0};
	st.pipeline = G_ResourceListPush(&g_device->pipelines, g_device->permanent_arena, &resource, hashed_key);
	st.layout = layout_key;
	st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	ScratchRelease(&scratch);
	
	return st;
}

internal G_PipelineSt G_DeviceFetchComputePipeline(const G_ComputePipelineDef *def)
{
	u64 hashed_key_value = HashBytesGeneric(def, sizeof(*def));
	G_ResourceKey hashed_key = { hashed_key_value };
	
	G_ResourceKey layout_key = G_DevicePipelineLayoutFetch(def->program);
	
	if (G_DevicePipelineFromKey(hashed_key))
	{
		G_PipelineSt st = {0};
		st.pipeline = hashed_key;
		st.layout = layout_key;
		st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
		
		return st;
	}
	
	VkPipelineLayout layout = G_DevicePipelineLayoutFromKey(layout_key);
	
	G_ShaderProgram *program = G_DeviceShaderProgramFromKey(def->program);

	DebugLogAssert(g_device->log_channel, G_ShaderProgramIsCompute(program), "Shader program must be a compute shader!");

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

	G_VK_CHECK(vkCreateComputePipelines(g_device->vk_device,
										g_device->pipeline_process_cache,
										1, &compute_pipeline_create_info,
										NULL, &pipeline),
			   "Failed to create compute pipeline.");

	G_Resource resource = {0};
	resource.pipeline = pipeline;
	
	G_PipelineSt st = {0};
	st.pipeline = G_ResourceListPush(&g_device->pipelines, g_device->permanent_arena, &resource, hashed_key);
	st.layout = layout_key;
	st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
	
	return st;
}

internal VkPipeline G_DevicePipelineFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->pipelines, key);
	return resource ? resource->pipeline : VK_NULL_HANDLE;
}

internal G_ResourceKey G_DeviceTextureAlloc(const G_TextureAllocInfo *alloc_info)
{
	G_Texture texture = {0};

	texture.width  = alloc_info->width;
	texture.height = alloc_info->height;
	texture.depth  = alloc_info->depth;

	texture.format = alloc_info->format;
	texture.type   = alloc_info->type;
	texture.tiling = alloc_info->tiling;

	texture.mipmap_count = G_ClampMipmapCount(alloc_info->mipmaps, alloc_info->width, alloc_info->height, alloc_info->depth);
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

	b32 is_depth   = (alloc_info->format == G_DeviceDepthFormat());
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
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = alloc_info->type;
	create_info.extent.width = texture.width;
	create_info.extent.height = texture.height;
	create_info.extent.depth = texture.depth;
	create_info.mipLevels = texture.mipmap_count;
	create_info.arrayLayers = texture.layer_count;
	create_info.format = texture.format;
	create_info.tiling = texture.tiling;
	create_info.usage = texture.usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = texture.sample_count;
	create_info.flags = vk_create_flags;

	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.priority = 1.f;
	vma_alloc_info.flags = vma_alloc_flags;

	G_VK_CHECK(vmaCreateImage(g_device->vma_allocator,
							  &create_info,
							  &vma_alloc_info,
							  &texture.vk_handle,
							  &texture.allocation,
							  &texture.allocation_info),
			   "Failed to allocate texture.");

	G_Resource resource = {0};
	resource.texture = texture;
	
	return G_ResourceListPushAuto(&g_device->textures, g_device->permanent_arena, &texture);
}

internal G_ResourceKey G_DeviceTextureAlloc2D(u32 width, u32 height, VkFormat format, u32 mipmaps)
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

	return G_DeviceTextureAlloc(&alloc_info);
}

internal G_ResourceKey G_DeviceTextureAlloc2DRW(u32 width, u32 height, VkFormat format, u32 mipmaps)
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

	return G_DeviceTextureAlloc(&alloc_info);
}

internal G_ResourceKey G_DeviceTextureAllocDepth2D(u32 width, u32 height, u32 mipmaps)
{
	return G_DeviceTextureAlloc2D(width, height, G_DeviceDepthFormat(), mipmaps);
}

internal G_ResourceKey G_DeviceTextureAllocDepth2DRW(u32 width, u32 height, u32 mipmaps)
{
	return G_DeviceTextureAlloc2DRW(width, height, G_DeviceDepthFormat(), mipmaps);
}

internal G_ResourceKey G_DeviceTextureAllocCubemap(u32 resolution, VkFormat format, u32 mipmaps)
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

	return G_DeviceTextureAlloc(&alloc_info);
}

internal G_ResourceKey G_DeviceTextureAllocCubemapDepth(u32 resolution, u32 mipmaps)
{
	return G_DeviceTextureAllocCubemap(resolution, G_DeviceDepthFormat(), mipmaps);
}

internal void G_DeviceTextureDestroy(G_ResourceKey key)
{
	G_Texture *texture = G_DeviceTextureFromKey(key);
	DebugLogAssert(g_device->log_channel, texture, "Invalid texture with key %llu when destroying.", key.value);

	G_DeviceFrameInFlight *frame_in_flight = &g_device->frames_in_flight[g_device->current_frame_in_flight_index];

	G_DestroyedImage *node = ArenaPushArray(&frame_in_flight->arena, G_DestroyedImage, 1);
	node->next = frame_in_flight->destroyed_image_head;
	frame_in_flight->destroyed_image_head = node;

	node->key = key;
	node->image = texture->vk_handle;
	node->allocation = texture->allocation;
}

internal G_Texture *G_DeviceTextureFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->textures, key);
	return resource ? &resource->texture : NULL;
}

internal G_ResourceKey G_DeviceTextureViewFetch(const G_TextureViewCreateInfo *info)
{
	G_ResourceKey hashed_key = { HashBytesGeneric(info, sizeof(*info)) };
	
	if (G_DeviceTextureViewFromKey(hashed_key))
		return hashed_key;
	
	G_Texture *gfx_texture = G_DeviceTextureFromKey(info->texture);
	DebugLogAssert(g_device->log_channel, gfx_texture, "Invalid texture with key %llu when creating view.", info->texture.value);
	
	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = gfx_texture->vk_handle;
	view_create_info.viewType = info->type;
	view_create_info.format = gfx_texture->format;

	view_create_info.subresourceRange.aspectMask = info->range.aspects;
	view_create_info.subresourceRange.baseMipLevel = info->range.base_mip;
	view_create_info.subresourceRange.levelCount = info->range.mips;
	view_create_info.subresourceRange.baseArrayLayer = info->range.base_layer;
	view_create_info.subresourceRange.layerCount = info->range.layers;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	G_TextureView view = {0};
	view.type = info->type;
	view.range = info->range;

	G_VK_CHECK(vkCreateImageView(g_device->vk_device,
								 &view_create_info, NULL,
								 &view.vk_handle),
			   "Failed to create texture image view.");

	b32 is_cubemap = (gfx_texture->flags & G_TextureFlag_Cubemap) != 0;
	b32 is_storage = (gfx_texture->flags & G_TextureFlag_Storage) != 0;

	view.bindless = G_BindlessRegisterView(&g_device->bindless, view.vk_handle, is_cubemap, is_storage);
	
	G_Resource resource = {0};
	resource.texture_view = view;
	
	return G_ResourceListPush(&g_device->texture_views, g_device->permanent_arena, &resource, hashed_key);
}

internal G_ResourceKey G_DeviceTextureViewAuto(G_ResourceKey texture)
{
	G_Texture *gfx_texture = G_DeviceTextureFromKey(texture);
	DebugLogAssert(g_device->log_channel, gfx_texture, "Invalid texture with key %llu when creating view (auto).", texture.value);

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

	return G_DeviceTextureViewFetch(&info);
}

internal G_TextureView *G_DeviceTextureViewFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->texture_views, key);
	return resource ? &resource->texture_view : NULL;
}

internal u32 G_DeviceTextureViewBindless(G_ResourceKey key)
{
	G_TextureView *gfx_view = G_DeviceTextureViewFromKey(key);
	DebugLogAssert(g_device->log_channel, gfx_view, "Invalid texture view with key %llu when getting bindless info.", key.value);
	
	return gfx_view->bindless;
}

internal G_ResourceKey G_DeviceBufferAlloc(const G_BufferAllocInfo *alloc_info)
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
	
	G_VK_CHECK(vmaCreateBuffer(g_device->vma_allocator,
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

		buffer.device_address = vkGetBufferDeviceAddress(g_device->vk_device, &address_info);
	}

	G_Resource resource = {0};
	resource.buffer = buffer;
	
	return G_ResourceListPushAuto(&g_device->buffers, g_device->permanent_arena, &resource);
}

internal G_ResourceKey G_DeviceStageAlloc(u64 size)
{
	G_BufferAllocInfo alloc_info = {0};
	alloc_info.usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
	alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	alloc_info.size = size;

	return G_DeviceBufferAlloc(&alloc_info);
}

internal void G_DeviceBufferDestroy(G_ResourceKey key)
{
	G_Buffer *buffer = G_DeviceBufferFromKey(key);
	DebugLogAssert(g_device->log_channel, buffer, "Invalid buffer with key %llu when destroying.", key.value);

	G_DeviceFrameInFlight *frame_in_flight = &g_device->frames_in_flight[g_device->current_frame_in_flight_index];

	G_DestroyedBuffer *node = ArenaPushArray(&frame_in_flight->arena, G_DestroyedBuffer, 1);
	node->next = frame_in_flight->destroyed_buffer_head;
	frame_in_flight->destroyed_buffer_head = node;

	node->key = key;
	node->buffer = buffer->vk_handle;
	node->allocation = buffer->allocation;
}

internal G_Buffer *G_DeviceBufferFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->buffers, key);
	return resource ? &resource->buffer : NULL;
}

internal void *G_DeviceBufferMap(G_ResourceKey key)
{
	G_Buffer *buffer = G_DeviceBufferFromKey(key);
	DebugLogAssert(g_device->log_channel, buffer, "Invalid buffer with key %llu when mapping memory.", key.value);
	
	return buffer->allocation_info.pMappedData;
}

internal u64 G_DeviceBufferAddress(G_ResourceKey key)
{
	G_Buffer *buffer = G_DeviceBufferFromKey(key);
	DebugLogAssert(g_device->log_channel, buffer, "Invalid buffer with key %llu when getting device address.", key.value);

	return buffer->device_address;
}

internal void G_DeviceBufferRead(G_ResourceKey key, void *dst, u64 length, u64 offset)
{
	G_Buffer *buffer = G_DeviceBufferFromKey(key);
	DebugLogAssert(g_device->log_channel, buffer, "Invalid buffer with key %llu when reading.", key.value);
	
	vmaCopyAllocationToMemory(g_device->vma_allocator, buffer->allocation, offset, dst, length);
}

internal void G_DeviceBufferWrite(G_ResourceKey key, const void *src, u64 length, u64 offset)
{
	G_Buffer *buffer = G_DeviceBufferFromKey(key);
	DebugLogAssert(g_device->log_channel, buffer, "Invalid buffer with key %llu when writing.", key.value);
	
	vmaCopyMemoryToAllocation(g_device->vma_allocator, src, buffer->allocation, offset, length);
}

internal u64 G_DeviceBufferSize(G_ResourceKey key)
{
	G_Buffer *buffer = G_DeviceBufferFromKey(key);
	DebugLogAssert(g_device->log_channel, buffer, "Invalid buffer with key %llu when getting size.", key.value);

	return buffer->size;
}

internal G_ResourceKey G_DeviceSamplerCreate(const G_SamplerCreateInfo *info)
{
	VkPhysicalDeviceProperties properties = g_device->vk_physical_device_properties.properties;

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
	
	G_VK_CHECK(vkCreateSampler(g_device->vk_device,
							   &create_info, NULL,
							   &sampler.vk_handle),
			   "Failed to create texture sampler.");

	sampler.filter = info->filter;
	sampler.wrap_x = info->wrap_x;
	sampler.wrap_y = info->wrap_y;
	sampler.wrap_z = info->wrap_z;
	sampler.border_colour = info->border_colour;
	sampler.bindless = G_BindlessRegisterSampler(&g_device->bindless, sampler.vk_handle);

	G_Resource resource = {0};
	resource.sampler = sampler;
	
	return G_ResourceListPushAuto(&g_device->samplers, g_device->permanent_arena, &resource);
}

internal G_ResourceKey G_DeviceSamplerCreateF(VkFilter filter)
{
	G_SamplerCreateInfo create_info = {0};
	create_info.filter = filter;
	create_info.wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.border_colour = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	return G_DeviceSamplerCreate(&create_info);
}

internal void G_DeviceSamplerDestroy(G_ResourceKey key)
{
	G_Sampler *sampler = G_DeviceSamplerFromKey(key);
	DebugLogAssert(g_device->log_channel, sampler, "Invalid sampler with key %llu when destroying.", key.value);

	G_DeviceFrameInFlight *frame_in_flight = &g_device->frames_in_flight[g_device->current_frame_in_flight_index];

	G_DestroyedSampler *node = ArenaPushArray(&frame_in_flight->arena, G_DestroyedSampler, 1);
	node->next = frame_in_flight->destroyed_sampler_head;
	frame_in_flight->destroyed_sampler_head = node;

	node->key = key;
	node->sampler = sampler->vk_handle;
	node->bindless = sampler->bindless;
}

internal G_Sampler *G_DeviceSamplerFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->samplers, key);
	return resource ? &resource->sampler : NULL;
}

internal u32 G_DeviceSamplerBindless(G_ResourceKey key)
{
	G_Sampler *gfx_sampler = G_DeviceSamplerFromKey(key);
	DebugLogAssert(g_device->log_channel, gfx_sampler, "Invalid sampler with key %llu when destroying.", key.value);
	
	return gfx_sampler->bindless;
}

internal G_ShaderStage G_DeviceShaderStageCreate(Arena *arena, const G_ShaderBytecode *bytecode)
{
	SpvReflectShaderModule reflect_module = {0};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(bytecode->size, bytecode->bytes, &reflect_module);

	DebugLogAssert(g_device->log_channel,
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
		DebugLogB(g_device->log_channel, "No entry points found in SPIR-V.");
	}

	spvReflectDestroyShaderModule(&reflect_module);

	ScratchRelease(&scratch);

	return stage;
}

internal G_ResourceKey G_DeviceShaderProgramCreate(u32 stage_count, const G_ShaderBytecode *stages)
{
	G_ShaderProgram program = {0};
	program.stage_count = stage_count;
	program.push_constant_size = 0;

	for (u32 i = 0; i < stage_count; i++)
	{
		program.stages[i] = G_DeviceShaderStageCreate(g_device->permanent_arena, &stages[i]);
		program.push_constant_size = MaxValue(program.push_constant_size, program.stages[i].push_constant_size);
	}

	G_Resource resource = {0};
	resource.shader = program;
	
	return G_ResourceListPushAuto(&g_device->shaders, g_device->permanent_arena, &resource);
}

internal void G_DeviceShaderProgramDestroy(G_ResourceKey key)
{
	G_ShaderProgram *shader = G_DeviceShaderProgramFromKey(key);
	DebugLogAssert(g_device->log_channel, shader, "Invalid shader program with key %llu when destroying.", key.value);

	G_DeviceFrameInFlight *frame_in_flight = &g_device->frames_in_flight[g_device->current_frame_in_flight_index];

	G_DestroyedShaderProgram *node = ArenaPushArray(&frame_in_flight->arena, G_DestroyedShaderProgram, 1);
	node->next = frame_in_flight->destroyed_shader_head;
	frame_in_flight->destroyed_shader_head = node;

	node->key = key;
}

internal G_ShaderProgram *G_DeviceShaderProgramFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->shaders, key);
	return resource ? &resource->shader : NULL;
}

internal G_DeviceAllocAccelStructReceipt G_DeviceBLASAlloc(const G_BLASGeometry *geometries, u32 geometry_count)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkAccelerationStructureGeometryKHR *vk_geometries = ArenaPushArray(scratch.arena, VkAccelerationStructureGeometryKHR, geometry_count);
	u32 *primitive_counts = ArenaPushArray(scratch.arena, u32, geometry_count);

	for (u32 i = 0; i < geometry_count; i++)
	{
		const G_BLASGeometry *geometry = &geometries[i];

		G_Buffer *vb = G_DeviceBufferFromKey(geometry->vertex_buffer);
		G_Buffer *ib = G_DeviceBufferFromKey(geometry->index_buffer);

		vk_geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vk_geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vk_geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureGeometryTrianglesDataKHR *tri = &vk_geometries[i].geometry.triangles;

		tri->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		tri->vertexFormat = geometry->vertex_format;
		tri->vertexData.deviceAddress = vb->device_address + geometry->vertex_offset;
		tri->vertexStride = geometry->vertex_stride;
		tri->maxVertex = geometry->vertex_count - 1;

		tri->indexType = geometry->index_type;
		tri->indexData.deviceAddress = ib->device_address + geometry->index_offset;

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

	vkGetAccelerationStructureBuildSizesKHR(g_device->vk_device,
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

	G_ResourceKey backing_buffer_key = G_DeviceBufferAlloc(&buf_alloc_info);
	G_Buffer *backing_buffer = G_DeviceBufferFromKey(backing_buffer_key);

	VkAccelerationStructureCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.buffer = backing_buffer->vk_handle;
	create_info.size = sizes.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	G_AccelStruct accel_struct = {0};
	accel_struct.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accel_struct.backing_buffer = backing_buffer_key;

	G_VK_CHECK(vkCreateAccelerationStructureKHR(g_device->vk_device,
												&create_info, NULL,
												&accel_struct.vk_handle),
			   "Failed to create BLAS.");

	VkAccelerationStructureDeviceAddressInfoKHR addr_info = {0}; // fuck me khronos shorter names would be appreciated yes??
	addr_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addr_info.accelerationStructure = accel_struct.vk_handle;

	accel_struct.device_address = vkGetAccelerationStructureDeviceAddressKHR(g_device->vk_device, &addr_info);
	
	ScratchRelease(&scratch);

	DebugLogD(g_device->log_channel, "Allocated BLAS from %u geometries.", geometry_count);

	G_Resource resource = {0};
	resource.accel_struct = accel_struct;
	
	G_DeviceAllocAccelStructReceipt receipt = {0};
	receipt.key = G_ResourceListPushAuto(&g_device->accel_structures, g_device->permanent_arena, &resource);
	receipt.scratch_size = sizes.buildScratchSize;
	
	return receipt;
}

internal G_DeviceAllocAccelStructReceipt G_DeviceTLASAlloc(u32 max_instance_count)
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

	vkGetAccelerationStructureBuildSizesKHR(g_device->vk_device,
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

	G_ResourceKey backing_buffer_key = G_DeviceBufferAlloc(&buf_alloc_info);
	G_Buffer *backing_buffer = G_DeviceBufferFromKey(backing_buffer_key);

	VkAccelerationStructureCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.buffer = backing_buffer->vk_handle;
	create_info.size = sizes.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	G_AccelStruct accel_struct = {0};
	accel_struct.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accel_struct.backing_buffer = backing_buffer_key;

	G_VK_CHECK(vkCreateAccelerationStructureKHR(g_device->vk_device,
												&create_info, NULL,
												&accel_struct.vk_handle),
			   "Failed to create TLAS.");

	VkAccelerationStructureDeviceAddressInfoKHR addr_info = {0}; // fuck me khronos shorter names would be appreciated yes??
	addr_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addr_info.accelerationStructure = accel_struct.vk_handle;

	accel_struct.device_address = vkGetAccelerationStructureDeviceAddressKHR(g_device->vk_device, &addr_info);

	DebugLogD(g_device->log_channel, "Allocated TLAS with max instance count of %u.", max_instance_count);
	
	G_Resource resource = {0};
	resource.accel_struct = accel_struct;
	
	G_DeviceAllocAccelStructReceipt receipt = {0};
	receipt.key = G_ResourceListPushAuto(&g_device->accel_structures, g_device->permanent_arena, &resource);
	receipt.scratch_size = sizes.buildScratchSize;
	
	return receipt;
}

internal void G_DeviceAccelStructDestroy(G_ResourceKey key)
{
	G_AccelStruct *accel_struct = G_DeviceAccelStructFromKey(key);
	DebugLogAssert(g_device->log_channel, accel_struct, "Invalid acceleration structure with key %llu when destroying.", key.value);
	
	G_DeviceFrameInFlight *frame_in_flight = &g_device->frames_in_flight[g_device->current_frame_in_flight_index];

	G_DeviceBufferDestroy(accel_struct->backing_buffer);

	G_DestroyedAccelStruct *node = ArenaPushArray(&frame_in_flight->arena, G_DestroyedAccelStruct, 1);
	node->next = frame_in_flight->destroyed_as_head;
	frame_in_flight->destroyed_as_head = node;

	node->key = key;
	node->handle = accel_struct->vk_handle;
}

internal u64 G_DeviceAccelStructAddress(G_ResourceKey key)
{
	G_AccelStruct *accel_struct = G_DeviceAccelStructFromKey(key);
	DebugLogAssert(g_device->log_channel, accel_struct, "Invalid acceleration structure with key %llu when getting device address.", key.value);

	return accel_struct->device_address;
}

internal G_AccelStruct *G_DeviceAccelStructFromKey(G_ResourceKey key)
{
	G_Resource *resource = G_ResourceListGet(&g_device->accel_structures, key);
	return resource ? &resource->accel_struct : NULL;
}

internal void G_DeviceCreateSyncResources(void)
{
	g_device->graphics_semaphore = G_DeviceSemaphoreCreate(0);

	u32 family_index = g_device->graphics_queue.family_index;

	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
	{
		G_DeviceFrameInFlight *frame = &g_device->frames_in_flight[i];
		
		frame->completion_point.frame = 0;
		frame->completion_point.semaphore = g_device->graphics_semaphore.vk_handle;

		frame->command_pool = G_DeviceCmdPoolCreate(family_index);

		// semaphore
		{
			VkSemaphoreCreateInfo binary_semaphore_create_info = {0};
			binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			binary_semaphore_create_info.flags = 0;
			binary_semaphore_create_info.pNext = NULL;

			G_VK_CHECK(vkCreateSemaphore(g_device->vk_device,
										 &binary_semaphore_create_info, NULL,
										 &frame->image_available_semaphore),
					   "Failed to create image available semaphore.");
		}
		
		frame->destroyed_image_head = NULL;
		frame->destroyed_buffer_head = NULL;
		frame->destroyed_sampler_head = NULL;
		frame->destroyed_shader_head = NULL;
		frame->destroyed_as_head = NULL;
	}

	DebugLogD(g_device->log_channel, "Created frame sync objects.");
}

internal void G_DeviceDestroySyncResources(void)
{
	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
	{
		G_DeviceFrameInFlight *frame = &g_device->frames_in_flight[i];
		
		vkDestroySemaphore(g_device->vk_device, frame->image_available_semaphore, NULL);		
		G_DeviceCmdPoolDestroy(&frame->command_pool);
	}

	G_DeviceSemaphoreDestroy(&g_device->graphics_semaphore);
}

internal void G_DeviceCreateBindless(void)
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

	G_VK_CHECK(vkCreateDescriptorPool(g_device->vk_device,
									  &pool_create_info, NULL,
									  &g_device->bindless.pool),
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

	G_VK_CHECK(vkCreateDescriptorSetLayout(g_device->vk_device,
										   &layout_create_info, NULL,
										   &g_device->bindless.layout),
			   "Failed to create bindless descriptor layout.");

	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = g_device->bindless.pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &g_device->bindless.layout;

	G_VK_CHECK(vkAllocateDescriptorSets(g_device->vk_device,
										&alloc_info,
										&g_device->bindless.set),
			   "Failed to allocate bindless descriptor set.");

	DebugLogD(g_device->log_channel, "Bindless resources created.");
}

internal void G_DeviceDestroyBindless(void)
{
	vkDestroyDescriptorSetLayout(g_device->vk_device, g_device->bindless.layout, NULL);
	vkDestroyDescriptorPool(g_device->vk_device, g_device->bindless.pool, NULL);
}

internal void G_DeviceApplyBindlessUpdates(void)
{
	if (g_device->bindless.update_count == 0)
		return;

	ScratchArena scratch = ScratchBegin(NULL, 0);

	u32 count = g_device->bindless.update_count;

	VkWriteDescriptorSet *writes = ArenaPushArray(scratch.arena, VkWriteDescriptorSet, count);
	VkDescriptorImageInfo *infos = ArenaPushArray(scratch.arena, VkDescriptorImageInfo, count);

	for (u32 i = 0; i < count; i++)
	{
		G_BindlessUpdate *update = &g_device->bindless.updates[i];

		infos[i].sampler = update->sampler;
		infos[i].imageView = update->view;
		infos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		writes[i] = (VkWriteDescriptorSet) {0};
		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].descriptorCount = 1;
		writes[i].dstArrayElement = update->slot;
		writes[i].descriptorType = G_BindlessGetVkType(update->kind);
		writes[i].dstSet = g_device->bindless.set;
		writes[i].dstBinding = update->kind;
		writes[i].pImageInfo = &infos[i];
	}

	vkUpdateDescriptorSets(g_device->vk_device,
						   count, writes,
						   0, NULL);

	g_device->bindless.update_count = 0;

	ScratchRelease(&scratch);
}

internal void G_DeviceCreateImGui(void)
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

	  G_VK_CHECK(vkCreateDescriptorPool(g_device->vk_device,
	  &pool_info, NULL,
	  &g_device->imgui_pool),
	  "Failed to create ImGui descriptor pool.");

	  VkFormat swapchain_image_format = VK_FORMAT_R32G32B32A32_SFLOAT;

	  ImGui_ImplVulkan_InitInfo init_info = {0};
	  init_info.Instance            = g_device->vk_instance;
	  init_info.PhysicalDevice      = g_device->vk_physical_device;
	  init_info.Device              = g_device->vk_device;
	  init_info.QueueFamily         = g_device->graphics_queue.family_index;
	  init_info.Queue               = g_device->graphics_queue.vk_handle;
	  init_info.PipelineCache       = g_device->pipeline_cache;
	  init_info.DescriptorPool      = g_device->imgui_pool;
	  init_info.Allocator           = NULL;
	  init_info.MinImageCount       = G_FRAMES_IN_FLIGHT;
	  init_info.ImageCount          = G_FRAMES_IN_FLIGHT;
	  init_info.CheckVkResultFn     = NULL;
	  init_info.UseDynamicRendering = true;
	  init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	  init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_image_format;

	  ImGui_ImplVulkan_Init(&init_info);
	*/
}

internal void G_DeviceDestroyImGui(void)
{
	/*
	  ImGui_ImplVulkan_Shutdown();
	  vkDestroyDescriptorPool(g_device->vk_device, g_device->imgui_pool, NULL);
	*/
}

internal void G_DeviceImGuiNewFrame(void)
{
	/*
	  ImGui_ImplVulkan_NewFrame();
	*/
}

internal void G_DeviceImGuiRecord(const G_CmdBuffer *cmd)
{
	/*
	  ImDrawData *draw_data = ImGui_GetDrawData();
	  ImGui_ImplVulkan_RenderDrawData(draw_data, cmd->vk_handle);
	*/
}
