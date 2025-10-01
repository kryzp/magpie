#include "device.h"

#include "ext/spirv_reflect.h"

#include "core/core_scratch.h"
#include "core/core_hash.h"

#include "platform/platform.h"

static const char *graphics_validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};

static VkFormat find_graphics_supported_format(VkPhysicalDevice physical_device,
					       VkImageTiling tiling,
					       VkFormatFeatureFlags features,
					       u32 candidate_count,
					       VkFormat *candidates)
{
	for (int i = 0; i < candidate_count; i++) {
		VkFormatProperties properties = {0};
		vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) ||
		    (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features))
			return candidates[i];
	}
	
	debug_log_crash("Failed to find supported format.");

	return VK_FORMAT_MAX_ENUM;
}

static VkFormat find_graphics_depth_format(VkPhysicalDevice physical_device)
{
	static VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	return find_graphics_supported_format(physical_device, VK_IMAGE_TILING_OPTIMAL,
					      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
					      array_size(candidates), candidates);
}

static VkSampleCountFlagBits find_graphics_max_usable_sample_count(VkPhysicalDeviceProperties2 properties)
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

	debug_log_crash("Could not find a maximum usable sample count.");
	
	return VK_SAMPLE_COUNT_1_BIT;
}

static const char *const *get_instance_extensions(struct memory_arena *arena, struct platform *platform, u32 *extension_count)
{
	const char *const *names = platform->get_vulkan_instance_extensions(extension_count);
	
	if (!names)
		debug_log_crash("Unable to get instance extension count.");

	u32 extra_extension_count = 3;

#ifdef __APPLE__
	extra_extension_count += 2;
#endif

	const char **extensions = memory_arena_array(arena,
						     *extension_count + extra_extension_count,
						     sizeof(const char *));

	for (int i = 0; i < *extension_count; i++)
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

static bool check_graphics_physical_device_extension_support(struct memory_arena *arena, VkPhysicalDevice physical_device)
{
	u32 extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL,
					     &extension_count, NULL);

	if (extension_count <= 0)
		debug_log_crash("Failed to find any device extension properties.");

	struct scratch_arena scratch = scratch_begin(arena, 1);

	VkExtensionProperties *available_exts = memory_arena_array(scratch.arena,
								   sizeof(VkExtensionProperties),
								   extension_count);
	
	vkEnumerateDeviceExtensionProperties(physical_device, NULL,
					     &extension_count, available_exts);

	bool result = true;

	for (int i = 0; i < extension_count; i++) {
		for (int j = 0; j < array_size(graphics_validation_layers); j++) {
			if (cstr_compare(available_exts[i].extensionName, graphics_validation_layers[j]) == 0) {
				result = false;
				goto exit;
			}
		}
	}

exit:
	scratch_release(&scratch);
	return result;
}

static bool check_for_validation_layer_support(struct memory_arena *arena)
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);

	struct scratch_arena scratch = scratch_begin(arena, 1);

	VkLayerProperties *available_layers = memory_arena_push(scratch.arena, sizeof(VkLayerProperties) * layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

	bool result = true;
	
	for (int i = 0; i < array_size(graphics_validation_layers); i++) {
		bool has_layer = false;
		const char *layer_name_0 = graphics_validation_layers[i];

		for (int j = 0; j < layer_count; j++) {
			const char *layer_name_1 = available_layers[j].layerName;

			if (cstr_compare(layer_name_0, layer_name_1) == 0) {
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
	scratch_release(&scratch);
	return result;
}

static struct gfx_swapchain_support_details query_swapchain_support(struct memory_arena *arena, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	struct gfx_swapchain_support_details result = {0};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &result.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &result.surface_format_count, NULL);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &result.present_mode_count, NULL);

	if (result.surface_format_count >= 0) {
		result.surface_formats = memory_arena_array(arena,
							    result.surface_format_count,
							    sizeof(VkSurfaceFormatKHR));

		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
						     &result.surface_format_count,
						     result.surface_formats);
	}

	if (result.present_mode_count >= 0) {
		result.present_modes = memory_arena_array(arena,
							  result.present_mode_count,
							  sizeof(VkPresentModeKHR));

		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
							  &result.present_mode_count,
							  result.present_modes);
	}

	return result;
}

static u32 assign_graphics_physical_device_usability(struct memory_arena *arena,
						     VkSurfaceKHR surface,
						     VkPhysicalDevice physical_device,
						     VkPhysicalDeviceProperties2 properties,
						     VkPhysicalDeviceFeatures2 features,
						     bool *has_essentials)
{
	u32 usability = 0;

	bool adequate_swap_chain = false;
	bool has_required_extensions = check_graphics_physical_device_extension_support(arena, physical_device);
	bool has_anisotropy = features.features.samplerAnisotropy;

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
		struct scratch_arena scratch = scratch_begin(arena, 1);

		struct gfx_swapchain_support_details details = query_swapchain_support(scratch.arena, physical_device, surface);

		adequate_swap_chain =
			(details.surface_format_count > 0) &&
			(details.present_mode_count > 0);

		usability += 3;

		scratch_release(&scratch);
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

static VKAPI_ATTR VkBool32 VKAPI_CALL graphics_vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
								     VkDebugUtilsMessageTypeFlagsEXT message_type,
								     const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
								     void *user_data)
{
	if (message_severity >=
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		debug_log_crash("Severity = %d, Type = %d, Message = \"%s\"",
				message_severity, message_type,
				callback_data->pMessage);
	}

	return VK_FALSE;
}

static VkResult create_graphics_device_debug_utils_messenger_ext(VkInstance instance,
								 VkDebugUtilsMessengerCreateInfoEXT *debug_info,
								 const VkAllocationCallbacks *allocator,
								 VkDebugUtilsMessengerEXT *messenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn)
		return fn(instance, debug_info, allocator, messenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static VkSurfaceFormatKHR _choose_swapchain_surface_format(VkSurfaceFormatKHR *available_surface_formats,
							   u32 available_surface_format_count)
{
	VkSurfaceFormatKHR *format = available_surface_formats;

	for (int i = 0; i < available_surface_format_count; i++, format++) {
		if (format->format == VK_FORMAT_B8G8R8A8_UNORM &&
		    format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			debug_log("Found desired swapchain swap surface format and colour space!");
			return *format;
		}
	}

	debug_log("Could not find desired swapchain swap surface format and colour space, falling back...");

	return *available_surface_formats;
}

static VkPresentModeKHR _choose_swapchain_present_mode(VkPresentModeKHR *available_present_modes,
						       u32 available_present_mode_count, b32 enable_vsync)
{
	if (!enable_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;

	VkPresentModeKHR *mode = available_present_modes;

	for (int i = 0; i < available_present_mode_count; i++, mode++) {
		if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D _choose_swapchain_extent(struct platform *platform, const VkSurfaceCapabilitiesKHR *capabilities)
{
	if (capabilities->currentExtent.width != (u32)(-1) &&
	    capabilities->currentExtent.height != (u32)(-1))
		return capabilities->currentExtent;

	VkExtent2D actual_extent = {
		platform->window_width,
		platform->window_height
	};

	actual_extent.width = clamp_value(actual_extent.width,
					  capabilities->minImageExtent.width,
					  capabilities->maxImageExtent.width);

	actual_extent.height = clamp_value(actual_extent.height,
					   capabilities->minImageExtent.height,
					   capabilities->maxImageExtent.height);

	return actual_extent;
}

void gfx_device_init(struct gfx_device *device, struct platform *platform, struct memory_arena *arena)
{
	device->arena = arena;
	
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

	struct scratch_arena scratch = scratch_begin(arena, 1);

	instance_create_info.ppEnabledExtensionNames = get_instance_extensions(scratch.arena,
									       platform,
									       &instance_create_info.enabledExtensionCount);

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {0};
	debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	debug_create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	debug_create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	debug_create_info.pfnUserCallback = graphics_vulkan_debug_callback;
	debug_create_info.pUserData = NULL;

	device->has_validation_layers = check_for_validation_layer_support(scratch.arena);

	if (device->has_validation_layers) {
		debug_log("Validation layer support verified.");

		instance_create_info.enabledLayerCount = array_size(graphics_validation_layers);
		instance_create_info.ppEnabledLayerNames = graphics_validation_layers;
		instance_create_info.pNext = &debug_create_info;
	} else {
		debug_log("No validation layer support.");

		instance_create_info.enabledLayerCount = 0;
		instance_create_info.ppEnabledLayerNames = NULL;
		instance_create_info.pNext = NULL;
	}

#ifdef __APPLE__
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	GFX_VK_CHECK(vkCreateInstance(&instance_create_info, NULL, &device->instance),
		     "Failed to create instance.");
	
	volkLoadInstance(device->instance);

	if (device->has_validation_layers) {
		GFX_VK_CHECK(create_graphics_device_debug_utils_messenger_ext(device->instance,
									      &debug_create_info, NULL,
									      &device->debug_messenger),
			     "Failed to create debug messenger.");
	}

	if (!platform->create_vulkan_surface(device->instance, &device->surface))
		debug_log_crash("Failed to create surface.");

	// Enumerate physical devices.
	{
		u32 device_count = 0;
		vkEnumeratePhysicalDevices(device->instance, &device_count, NULL);

		if (device_count <= 0)
			debug_log_crash("Failed to find GPUs with Vulkan support.");

		VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		VkPhysicalDeviceFeatures2 features     = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

		VkPhysicalDevice *devices = memory_arena_array(scratch.arena,
							       device_count,
							       sizeof(VkPhysicalDevice));
		
		vkEnumeratePhysicalDevices(device->instance, &device_count, devices);

		u32 best_usability = 0;
		u32 selected_id = 0;

		for (int i = 0; i < device_count; i++) {
			vkGetPhysicalDeviceProperties2(devices[i], &properties);
			vkGetPhysicalDeviceFeatures2(devices[i], &features);
			
			debug_log("Querying physical device: %s (%d)", properties.properties.deviceName, properties.properties.deviceID);

			bool has_essentials = false;
			u32 current_usability = assign_graphics_physical_device_usability(scratch.arena,
											  device->surface,
											  devices[i],
											  properties, features,
											  &has_essentials);

			if (current_usability > best_usability && has_essentials) {
				device->physical_device = devices[i];
				device->physical_device_properties = properties;
				device->physical_device_features = features;

				best_usability = current_usability;
				selected_id = properties.properties.deviceID;
			}
		}
		
		if (!device->physical_device)
			debug_log_crash("Unable to find a suitable GPU.");
		
		debug_log("Selected a suitable GPU: %d", selected_id);
	}

	device->max_msaa_samples = find_graphics_max_usable_sample_count(device->physical_device_properties);
	device->depth_format = find_graphics_depth_format(device->physical_device);

	// Locate the graphics queue.
	{
		u32 queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_count, 0);

		if (queue_family_count <= 0)
			debug_log_crash("Failed to find any queue families.");

		VkQueueFamilyProperties *queue_families = memory_arena_array(scratch.arena,
									     queue_family_count,
									     sizeof(VkQueueFamilyProperties));
		
		vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device,
							 &queue_family_count,
							 queue_families);

		for (int i = 0; i < queue_family_count; i++) {
			if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
				VkBool32 present_support = VK_FALSE;

				vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device, i,
								     device->surface,
								     &present_support);

				if (present_support) {
					device->graphics_queue.family_index = i;
					break;
				}

				continue;
			}
		}
	}

	float queue_priority = 1.f;

	VkDeviceQueueCreateInfo graphics_queue_create_info = {0};
	graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphics_queue_create_info.queueFamilyIndex = device->graphics_queue.family_index;
	graphics_queue_create_info.queueCount = 1;
	graphics_queue_create_info.pQueuePriorities = &queue_priority;

	// Disable this so we get a clear indication if something's gone wrong.
	device->physical_device_features.features.robustBufferAccess = VK_FALSE;

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
	vulkan12_features.pNext = &vulkan11_features;

	VkPhysicalDeviceVulkan13Features vulkan13_features = {0};
	vulkan13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13_features.dynamicRendering = VK_TRUE;
	vulkan13_features.synchronization2 = VK_TRUE;
	vulkan13_features.pNext = &vulkan12_features;

	static const char *device_extensions[] = {
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
	device_create_info.enabledExtensionCount = array_size(device_extensions);
	device_create_info.ppEnabledExtensionNames = device_extensions;
	device_create_info.pEnabledFeatures = &device->physical_device_features.features;
	device_create_info.pNext = &vulkan13_features;

	if (device->has_validation_layers) {
		device_create_info.enabledLayerCount = array_size(graphics_validation_layers);
		device_create_info.ppEnabledLayerNames = graphics_validation_layers;
		debug_log("Enabled validation layers.");
	}

	GFX_VK_CHECK(vkCreateDevice(device->physical_device,
				    &device_create_info, NULL,
				    &device->device),
		     "Failed to create logical device.");

	vkGetDeviceQueue(device->device,
			 device->graphics_queue.family_index, 0,
			 &device->graphics_queue.handle);

	debug_log("Created logical device.");

	VkSemaphoreCreateInfo semaphore_create_info = {0};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < GFX_FRAMES_IN_FLIGHT; i++) {
		device->frames[i].command_pool = gfx_device_command_pool_create(device, device->graphics_queue.family_index);
		
		VkFenceCreateInfo in_flight_fence_create_info = {0};
		in_flight_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		in_flight_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		GFX_VK_CHECK(vkCreateFence(device->device,
					   &in_flight_fence_create_info, NULL,
					   &device->frames[i].in_flight_fence),
			     "Failed to create queue frame in flight fence.");

		VkFenceCreateInfo instant_submit_fence_create_info = {0};
		instant_submit_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		instant_submit_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		GFX_VK_CHECK(vkCreateFence(device->device,
					   &instant_submit_fence_create_info, NULL,
					   &device->frames[i].instant_submit_fence),
			     "Failed to create queue frame instant submit fence.");

		GFX_VK_CHECK(vkCreateSemaphore(device->device,
					       &semaphore_create_info, NULL,
					       &device->frames[i].image_available_semaphore),
			     "Failed to create image available semaphore.");

		GFX_VK_CHECK(vkCreateSemaphore(device->device,
					       &semaphore_create_info, NULL,
					       &device->frames[i] .render_finished_semaphore),
			     "Failed to create render finished semaphore.");
	}

	debug_log("Created frame sync objects.");

	u32 version = 0;
	VkResult result = vkEnumerateInstanceVersion(&version);

	if (result == VK_SUCCESS) {
		u32 major = VK_API_VERSION_MAJOR(version);
		u32 minor = VK_API_VERSION_MINOR(version);
		debug_log("Using Vulkan %d.%d", major, minor);
	} else {
		debug_log("Failed to retrieve Vulkan version.");
	}

	volkLoadDevice(device->device);

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
	allocator_create_info.physicalDevice = device->physical_device;
	allocator_create_info.device = device->device;
	allocator_create_info.instance = device->instance;
	allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	//vmaImportVulkanFunctionsFromVolk(&allocator_create_info, &vulkan_functions);

	allocator_create_info.pVulkanFunctions = &vulkan_functions;

	GFX_VK_CHECK(vmaCreateAllocator(&allocator_create_info, &device->vma_allocator),
		     "Failed to create Vulkan Memory Allocator.");

	debug_log("Created Vulkan Memory Allocator.");

	VkPipelineCacheCreateInfo pipeline_cache_create_info = {0};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = NULL;
	pipeline_cache_create_info.flags = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = NULL;

	GFX_VK_CHECK(vkCreatePipelineCache(device->device,
					   &pipeline_cache_create_info, NULL,
					   &device->pipeline_process_cache),
		     "Failed to process pipeline cache.");

	debug_log("Created graphics pipeline process cache.");

	device->bindless = gfx_device_bindless_create(device);
	
	device->swapchain_details = query_swapchain_support(device->arena, device->physical_device, device->surface);

	hash_table_init(&device->texture_view_cache,    device->arena, sizeof(struct gfx_texture_view));
	hash_table_init(&device->pipeline_cache,        device->arena, sizeof(VkPipeline));
	hash_table_init(&device->pipeline_layout_cache, device->arena, sizeof(VkPipelineLayout));

	scratch_release(&scratch);
}

void gfx_device_destroy(struct gfx_device *device, struct platform *platform)
{
	// Destroy cached image views.
	for (int i = 0; i < array_size(device->texture_view_cache.buckets); i++) {
		if (device->texture_view_cache.buckets[i]) {
			struct hash_table_node *node = device->texture_view_cache.buckets[i];
	
			while (node) {
				gfx_device_texture_view_destroy(device, (struct gfx_texture_view *)node->data);
				node = node->next;
			}
		}
	}

	// Destroy cached pipeline layouts.
	for (int i = 0; i < array_size(device->pipeline_layout_cache.buckets); i++) {
		if (device->pipeline_layout_cache.buckets[i]) {
			struct hash_table_node *node = device->pipeline_layout_cache.buckets[i];

			while (node) {
				gfx_device_pipeline_layout_destroy(device, *((VkPipelineLayout *)node->data));
				node = node->next;
			}
		}
	}

	// Destroy cached pipelines.
	for (int i = 0; i < array_size(device->pipeline_cache.buckets); i++) {
		if (device->pipeline_cache.buckets[i]) {
			struct hash_table_node *node = device->pipeline_cache.buckets[i];

			while (node) {
				gfx_device_pipeline_destroy(device, *((VkPipeline *)node->data));
				node = node->next;
			}
		}
	}
	
	// Clean up frame synchronization objects.
	for (int i = 0; i < GFX_FRAMES_IN_FLIGHT; i++) {
		gfx_device_command_pool_destroy(device, &device->frames[i].command_pool);

		vkDestroyFence(device->device, device->frames[i].in_flight_fence, NULL);
		vkDestroyFence(device->device, device->frames[i].instant_submit_fence, NULL);

		vkDestroySemaphore(device->device, device->frames[i].render_finished_semaphore, NULL);
		vkDestroySemaphore(device->device, device->frames[i].image_available_semaphore, NULL);
	}

	gfx_device_bindless_destroy(device, &device->bindless);

	vkDestroyPipelineCache(device->device, device->pipeline_process_cache, NULL);
	platform->destroy_vulkan_surface(device->instance, device->surface);
	vmaDestroyAllocator(device->vma_allocator);
	vkDestroyDebugUtilsMessengerEXT(device->instance, device->debug_messenger, NULL);
	vkDestroyDevice(device->device, NULL);
}

// TODO: We really shouldn't need to initialize
//       and de-initialize volk like this. Surely
//       we should load all fpointers into a table
//       which we store in program memory (possible
//       in volk) and just load that table back
//       in after reloading?
//       --> Also I'm 99% certain that VMA's function
//           pointers also break down here, and I'm
//           not setting them back... Need to test.

void gfx_device_hot_load(struct gfx_device *device)
{
	volkInitialize();
	volkLoadInstance(device->instance);
	volkLoadDevice(device->device);

	gfx_device_wait_idle(device);
}

void gfx_device_hot_unload(struct gfx_device *device)
{
	volkFinalize();
}

VkSemaphore gfx_device_current_render_finished_semaphore(struct gfx_device *device)
{
	return device->frames[device->current_frame_index].render_finished_semaphore;
}

VkSemaphore gfx_device_current_image_available_semaphore(struct gfx_device *device)
{
	return device->frames[device->current_frame_index].image_available_semaphore;
}

void gfx_device_wait_idle(struct gfx_device *device)
{
	vkDeviceWaitIdle(device->device);
}

void gfx_device_wait_for_fence(struct gfx_device *device, VkFence fence)
{
	vkWaitForFences(device->device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void gfx_device_reset_fence(struct gfx_device *device, VkFence fence)
{
	vkResetFences(device->device, 1, &fence);
}

struct gfx_command_buffer gfx_device_begin_present(struct gfx_device *device, struct gfx_swapchain *swapchain)
{
	struct gfx_sync_data *current_frame = device->frames + device->current_frame_index;

	gfx_device_wait_for_fence(device, current_frame->in_flight_fence);
	gfx_device_reset_fence(device, current_frame->in_flight_fence);

	gfx_device_swapchain_acquire_next_image(device, swapchain);
	
	gfx_device_command_pool_reset(device, &current_frame->command_pool);

	struct gfx_command_buffer cmd = gfx_command_pool_fetch_free(&current_frame->command_pool);

	gfx_cmd_begin(&cmd);

	return cmd;
}

void gfx_device_end_present(struct gfx_device *device, struct gfx_swapchain *swapchain, struct gfx_command_buffer *cmd)
{
	struct gfx_sync_data *current_frame = device->frames + device->current_frame_index;
	
	gfx_device_bindless_apply_updates(device, &device->bindless);

	gfx_cmd_end(cmd);

	VkFence fence = current_frame->in_flight_fence;

	VkSemaphoreSubmitInfo render_finished_semaphore = {0};
	render_finished_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore.semaphore = gfx_device_current_render_finished_semaphore(device);
	render_finished_semaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSemaphoreSubmitInfo image_available_semaphore = {0};
	image_available_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore.semaphore = gfx_device_current_image_available_semaphore(device);
	image_available_semaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkCommandBufferSubmitInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = cmd->handle;

	VkSubmitInfo2 submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;
	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;
	submit_info.signalSemaphoreInfoCount = 1;
	submit_info.pSignalSemaphoreInfos = &render_finished_semaphore;
	submit_info.waitSemaphoreInfoCount = 1;
	submit_info.pWaitSemaphoreInfos = &image_available_semaphore;

	gfx_queue_submit(&device->graphics_queue, &submit_info, fence);

	u32 texture_index = swapchain->current_texture_index;

	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = NULL;
	present_info.pImageIndices = &texture_index;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphore.semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain->handle;

	VkResult result = gfx_queue_present(&device->graphics_queue, &present_info);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		debug_log_crash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		debug_log_crash("Failed to present swapchain image.");

	device->current_frame_index = (device->current_frame_index + 1) % GFX_FRAMES_IN_FLIGHT;

	gfx_queue_wait_idle(&device->graphics_queue);

}

struct gfx_command_buffer gfx_device_begin_instant_submit(struct gfx_device *device)
{
	struct gfx_sync_data *current_frame = device->frames + device->current_frame_index;

	gfx_device_wait_for_fence(device, current_frame->instant_submit_fence);
	gfx_device_reset_fence(device, current_frame->instant_submit_fence);

	struct gfx_command_buffer cmd = gfx_command_pool_fetch_free(&current_frame->command_pool);

	gfx_cmd_begin(&cmd);

	return cmd;
}

void gfx_device_end_instant_submit(struct gfx_device *device, struct gfx_command_buffer *cmd)
{
	struct gfx_sync_data *current_frame = device->frames + device->current_frame_index;

	gfx_cmd_end(cmd);

	VkCommandBufferSubmitInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = cmd->handle;

	VkSubmitInfo2 submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;
	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;
	submit_info.signalSemaphoreInfoCount = 0;
	submit_info.pSignalSemaphoreInfos = NULL;
	submit_info.waitSemaphoreInfoCount = 0;
	submit_info.pWaitSemaphoreInfos = NULL;

	gfx_queue_submit(&device->graphics_queue, &submit_info, current_frame->instant_submit_fence);
}

struct gfx_swapchain gfx_device_swapchain_create(struct gfx_device *device, struct platform *platform)
{
	struct memory_arena *arena = device->arena;
	
	struct scratch_arena scratch = scratch_begin(arena, 1);

	struct gfx_swapchain_support_details details = device->swapchain_details;
	
	VkSurfaceFormatKHR surface_format = _choose_swapchain_surface_format(details.surface_formats, details.surface_format_count);
	VkPresentModeKHR present_mode = _choose_swapchain_present_mode(details.present_modes, details.present_mode_count, true);
	VkExtent2D extent = _choose_swapchain_extent(platform, &details.capabilities);
	
	struct gfx_swapchain swapchain = {0};
	
	swapchain.width = extent.width;
	swapchain.height = extent.height;
	swapchain.format = surface_format.format;

	u32 texture_count = details.capabilities.minImageCount + 1;

	if (details.capabilities.maxImageCount > 0 && texture_count > details.capabilities.maxImageCount)
		texture_count = details.capabilities.maxImageCount;

	const VkImageUsageFlags swapchain_texture_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	
	VkSwapchainCreateInfoKHR create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = device->surface;
	create_info.minImageCount = texture_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = swapchain_texture_usage;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = NULL;
	create_info.preTransform = details.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	GFX_VK_CHECK(vkCreateSwapchainKHR(device->device,
					  &create_info, NULL,
					  &swapchain.handle),
		     "Failed to create swapchain.");

	vkGetSwapchainImagesKHR(device->device, swapchain.handle, &texture_count, NULL);

	if (texture_count <= 0)
		debug_log_crash("Failed to find any images in swapchain.");

	swapchain.swapchain_texture_count = texture_count;

	swapchain.swapchain_textures = memory_arena_array(arena, texture_count, sizeof(struct gfx_texture));
	swapchain.swapchain_views    = memory_arena_array(arena, texture_count, sizeof(struct gfx_texture_view));

	VkImage *vk_images = memory_arena_array(scratch.arena, sizeof(VkImage), texture_count);

	vkGetSwapchainImagesKHR(device->device, swapchain.handle, &texture_count, vk_images);

	for (int i = 0; i < texture_count; i++) {
		struct gfx_texture *texture = swapchain.swapchain_textures + i;

		texture->handle = vk_images[i];

		texture->access_types = memory_arena_array(arena, 1, sizeof(enum gfx_texture_access_type));
		
		texture->width = swapchain.width;
		texture->height = swapchain.height;
		texture->depth = 1;

		texture->is_swapchain = true;

		texture->format = swapchain.format;
		texture->type   = VK_IMAGE_VIEW_TYPE_2D;
		texture->tiling = VK_IMAGE_TILING_OPTIMAL;

		texture->usage = swapchain_texture_usage;

		texture->aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
		
		texture->aspect_count = 1;
		texture->mipmap_count = 1;
		
		texture->samples = VK_SAMPLE_COUNT_1_BIT;

		swapchain.swapchain_views[i] = gfx_device_texture_view_create(device, texture,
									      gfx_texture_layer_count(texture),
									      0, 0);
	}

	scratch_release(&scratch);

	debug_log("Swapchain created.");

	return swapchain;
}

void gfx_device_swapchain_destroy(struct gfx_device *device, struct gfx_swapchain *swapchain)
{
	for (int i = 0; i < swapchain->swapchain_texture_count; i++)
		gfx_device_texture_view_destroy(device, swapchain->swapchain_views + i);

	vkDestroySwapchainKHR(device->device, swapchain->handle, NULL);
}

void gfx_device_swapchain_acquire_next_image(struct gfx_device *device, struct gfx_swapchain *swapchain)
{
	VkAcquireNextImageInfoKHR info = {0};
	info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	info.swapchain = swapchain->handle;
	info.timeout = UINT64_MAX;
	info.semaphore = gfx_device_current_image_available_semaphore(device);
	info.fence = VK_NULL_HANDLE;
	info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(device->device, &info, &swapchain->current_texture_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		debug_log_crash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		debug_log_crash("Failed to acquire next image in swapchain.");
}

static VkDescriptorType gfx_bindless_get_descriptor_type_from_set(enum gfx_bindless_set_kind kind)
{
	switch (kind) {
	case GFX_BINDLESS_SET_sampler:  return VK_DESCRIPTOR_TYPE_SAMPLER;
	case GFX_BINDLESS_SET_sampled:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case GFX_BINDLESS_SET_storage:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}

	debug_log_crash("Could not find descriptor type from bindless binding type.");

	return (VkDescriptorType)0;
}

struct gfx_bindless gfx_device_bindless_create(struct gfx_device *device)
{
	struct gfx_bindless bindless = {0};

	VkDescriptorPoolSize pool_sizes[GFX_BINDLESS_SET_max_enum] = {0};
	
	for (u32 i = 0; i < GFX_BINDLESS_SET_max_enum; i++) {
		pool_sizes[i].type = gfx_bindless_get_descriptor_type_from_set(i);
		pool_sizes[i].descriptorCount = GFX_BINDLESS_MAX_RESOURCES;
	}
	
	VkDescriptorPoolCreateInfo pool_create_info = {0};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = GFX_BINDLESS_MAX_RESOURCES * array_size(pool_sizes);
	pool_create_info.poolSizeCount = array_size(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;
	
	GFX_VK_CHECK(vkCreateDescriptorPool(device->device,
					    &pool_create_info, NULL,
					    &bindless.pool),
		     "Failed to create bindless descriptor pool.");
	
	VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {0};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = 1;
	binding_flags.pBindingFlags = &bindless_flags;
	
	for (u32 i = 0; i < GFX_BINDLESS_SET_max_enum; i++) {
		VkDescriptorSetLayoutBinding binding = {0};
		binding.descriptorType = gfx_bindless_get_descriptor_type_from_set(i);
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
		
		GFX_VK_CHECK(vkCreateDescriptorSetLayout(device->device,
							 &layout_create_info, NULL,
							 &bindless.layouts[i]),
			     "Failed to create bindless descriptor layout.");
	}
	
	// ---
	
	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = bindless.pool;
	alloc_info.descriptorSetCount = GFX_BINDLESS_SET_max_enum;
	alloc_info.pSetLayouts = bindless.layouts;
	
	GFX_VK_CHECK(vkAllocateDescriptorSets(device->device, &alloc_info, bindless.sets),
		     "Failed to allocate bindless descriptor set.");
	
	debug_log("Bindless resources created.");

	return bindless;
}

void gfx_device_bindless_destroy(struct gfx_device *device, struct gfx_bindless *bindless)
{
	for (u32 i = 0; i < GFX_BINDLESS_SET_max_enum; i++)
		vkDestroyDescriptorSetLayout(device->device, bindless->layouts[i], NULL);
	
	vkDestroyDescriptorPool(device->device, bindless->pool, NULL);
}

void gfx_device_bindless_apply_updates(struct gfx_device *device, struct gfx_bindless *bindless)
{
	if (bindless->update_count <= 0)
		return;
	
	VkWriteDescriptorSet descriptor_writes[GFX_BINDLESS_MAX_WRITES_PER_FRAME] = {0};
	VkDescriptorImageInfo image_infos[GFX_BINDLESS_MAX_WRITES_PER_FRAME] = {0};

	for (u32 i = 0; i < bindless->update_count; i++) {
		struct gfx_bindless_update *update = bindless->updates + i;

		VkDescriptorImageInfo *image_info = image_infos + i;
		image_info->sampler = update->sampler;
		image_info->imageView = update->view;
		image_info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		
		VkWriteDescriptorSet *write = descriptor_writes + i;
		write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write->descriptorCount = 1;
		write->dstArrayElement = update->slot;
		write->descriptorType = gfx_bindless_get_descriptor_type_from_set(update->kind);
		write->dstSet = bindless->sets[update->kind];
		write->dstBinding = 0;
		write->pImageInfo = image_info;
	}

	vkUpdateDescriptorSets(device->device,
			       bindless->update_count, descriptor_writes,
			       0, NULL);
		
	bindless->update_count = 0;
}

VkPipelineLayout gfx_device_pipeline_layout_create(struct gfx_device *device, struct gfx_shader_program *program)
{
	VkShaderStageFlags stage = gfx_shader_program_is_compute(program)
		? VK_SHADER_STAGE_COMPUTE_BIT
		: VK_SHADER_STAGE_ALL_GRAPHICS;

	VkPushConstantRange push_constants = {0};
	push_constants.offset = 0;
	push_constants.size = program->push_constant_size;
	push_constants.stageFlags = stage;

	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = array_size(device->bindless.layouts);
	create_info.pSetLayouts = device->bindless.layouts;
	
	if (push_constants.size > 0) {
		create_info.pushConstantRangeCount = 1;
		create_info.pPushConstantRanges = &push_constants;
	} else {
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = NULL;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	GFX_VK_CHECK(vkCreatePipelineLayout(device->device,
					    &create_info, NULL,
					    &layout),
		     "Failed to create pipeline layout.");

	return layout;
}

void gfx_device_pipeline_layout_destroy(struct gfx_device *device, VkPipelineLayout layout)
{
	vkDestroyPipelineLayout(device->device, layout, NULL);
}

VkPipelineLayout gfx_device_pipeline_layout_fetch(struct gfx_device *device, struct gfx_shader_program *program)
{
	bool is_compute = gfx_shader_program_is_compute(program);

	u64 hash = 0;
	hash = hash_bytes_generic_combine(hash, &is_compute,                  sizeof(bool));
	hash = hash_bytes_generic_combine(hash, &program->push_constant_size, sizeof(u32));

	VkPipelineLayout *fetched_layout = hash_table_fetch(&device->pipeline_layout_cache, hash);

	if (fetched_layout)
		return *fetched_layout;

	VkPipelineLayout layout = gfx_device_pipeline_layout_create(device, program);

	hash_table_add(&device->pipeline_layout_cache, hash, &layout);

	return layout;
}

VkPipeline gfx_device_pipeline_create_graphics(struct gfx_device *device,
					       VkPipelineLayout layout,
					       struct gfx_graphics_pipeline_def *def)
{
	static const VkDynamicState graphics_pipeline_dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_BLEND_CONSTANTS // TODO: Add dynamic blend constants.
	};
	
	// We use vertex pulling in shaders so explicitly defined vertex formats aren't used.
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {0};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_state_create_info.pVertexBindingDescriptions = NULL;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_create_info.pVertexAttributeDescriptions = NULL;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {0};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = NULL; // Using dynamic viewport.
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = NULL; // Using dynamic scissor.

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

	VkPipelineColorBlendAttachmentState blend_states[GFX_MAX_COLOUR_ATTACHMENTS] = {0};

	VkPipelineColorBlendAttachmentState *blend_state = blend_states;

	for (int i = 0; i < def->colour_attachment_count; i++, blend_state++) {

		blend_state->blendEnable = def->blend_state.enabled;

		blend_state->srcColorBlendFactor = def->blend_state.colour.src;
		blend_state->dstColorBlendFactor = def->blend_state.colour.dst;
		blend_state->colorBlendOp = def->blend_state.colour.op;

		blend_state->srcAlphaBlendFactor = def->blend_state.alpha.src;
		blend_state->dstAlphaBlendFactor = def->blend_state.alpha.dst;
		blend_state->alphaBlendOp = def->blend_state.alpha.op;

		if (def->blend_state.write_mask[0]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (def->blend_state.write_mask[1]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (def->blend_state.write_mask[2]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (def->blend_state.write_mask[3]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
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
	depth_stencil_state_create_info.depthTestEnable = def->depth_stencil_state.depth_test_enabled;
	depth_stencil_state_create_info.depthWriteEnable = def->depth_stencil_state.depth_write_enabled;
	depth_stencil_state_create_info.depthCompareOp = def->depth_stencil_state.depth_compare_op;
	depth_stencil_state_create_info.depthBoundsTestEnable = def->depth_stencil_state.depth_bounds_test_enabled;
	depth_stencil_state_create_info.minDepthBounds = def->depth_stencil_state.depth_bounds_min;
	depth_stencil_state_create_info.maxDepthBounds = def->depth_stencil_state.depth_bounds_max;
	depth_stencil_state_create_info.stencilTestEnable = def->depth_stencil_state.stencil_test_enabled;
	depth_stencil_state_create_info.front.failOp = def->depth_stencil_state.stencil_front.fail_op;
	depth_stencil_state_create_info.front.passOp = def->depth_stencil_state.stencil_front.pass_op;
	depth_stencil_state_create_info.front.depthFailOp = def->depth_stencil_state.stencil_front.depth_fail_op;
	depth_stencil_state_create_info.front.compareOp = def->depth_stencil_state.stencil_front.compare_op;
	depth_stencil_state_create_info.front.writeMask = def->depth_stencil_state.stencil_front.write_mask;
	depth_stencil_state_create_info.front.reference = def->depth_stencil_state.stencil_front.reference;
	depth_stencil_state_create_info.back.failOp = def->depth_stencil_state.stencil_back.fail_op;
	depth_stencil_state_create_info.back.passOp = def->depth_stencil_state.stencil_back.pass_op;
	depth_stencil_state_create_info.back.depthFailOp = def->depth_stencil_state.stencil_back.depth_fail_op;
	depth_stencil_state_create_info.back.compareOp = def->depth_stencil_state.stencil_back.compare_op;
	depth_stencil_state_create_info.back.writeMask = def->depth_stencil_state.stencil_back.write_mask;
	depth_stencil_state_create_info.back.reference = def->depth_stencil_state.stencil_back.reference;

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {0};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = array_size(graphics_pipeline_dynamic_states);
	dynamic_state_create_info.pDynamicStates = graphics_pipeline_dynamic_states;

	VkFormat depth_stencil_format = def->has_depth_attachment
		? device->depth_format
		: VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {0};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.viewMask = def->view_mask;
	pipeline_rendering_create_info.colorAttachmentCount = def->colour_attachment_count;
	pipeline_rendering_create_info.pColorAttachmentFormats = def->colour_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat = depth_stencil_format;
	pipeline_rendering_create_info.stencilAttachmentFormat = depth_stencil_format;

	VkPipelineShaderStageCreateInfo shader_stages[2] = {0};

	for (int i = 0; i < def->program->stage_count; i++) {
		VkPipelineShaderStageCreateInfo *shader_stage = shader_stages + i;

		shader_stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage->stage = def->program->stages[i].stage;
		shader_stage->module = def->program->stages[i].module;
		shader_stage->pName = "main";
	}
	
	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {0};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = def->program->stage_count;
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

	GFX_VK_CHECK(vkCreateGraphicsPipelines(device->device,
					       device->pipeline_process_cache, 1,
					       &graphics_pipeline_create_info, NULL,
					       &pipeline),
		     "Failed to create graphics pipeline.");

	return pipeline;
}

VkPipeline gfx_device_pipeline_create_compute(struct gfx_device *device,
					      VkPipelineLayout layout,
					      struct gfx_compute_pipeline_def *def)
{
	VkPipelineShaderStageCreateInfo shader_stage = {0};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage = def->program->stages[0].stage;
	shader_stage.module = def->program->stages[0].module;
	shader_stage.pName = "main";

	VkComputePipelineCreateInfo compute_pipeline_create_info = {0};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.stage = shader_stage;

	VkPipeline pipeline = VK_NULL_HANDLE;

	GFX_VK_CHECK(vkCreateComputePipelines(device->device,
					      device->pipeline_process_cache, 1,
					      &compute_pipeline_create_info, NULL,
					      &pipeline),
		     "Failed to create compute pipeline.");

	return pipeline;
}

void gfx_device_pipeline_destroy(struct gfx_device *device, VkPipeline pipeline)
{
	vkDestroyPipeline(device->device, pipeline, NULL);
}

struct gfx_pipeline_st gfx_device_pipeline_fetch_graphics(struct gfx_device *device, struct gfx_graphics_pipeline_def *definition)
{
	VkPipelineLayout layout = gfx_device_pipeline_layout_fetch(device, definition->program);

	u64 hash = 0;
	hash = hash_bytes_generic_combine(hash, definition->program,                     sizeof(struct gfx_shader_program));
	hash = hash_bytes_generic_combine(hash, &definition->cull_mode,                  sizeof(VkCullModeFlags));
	hash = hash_bytes_generic_combine(hash, &definition->front_face,                 sizeof(VkFrontFace));
	hash = hash_bytes_generic_combine(hash, &definition->blend_state,                sizeof(struct gfx_blend_st));
	hash = hash_bytes_generic_combine(hash, &definition->depth_stencil_state,        sizeof(struct gfx_depth_stencil_st));
	hash = hash_bytes_generic_combine(hash, &definition->colour_attachment_count,    sizeof(u32));
	hash = hash_bytes_generic_combine(hash, &definition->colour_attachment_formats,  sizeof(VkFormat) * GFX_MAX_COLOUR_ATTACHMENTS);
	hash = hash_bytes_generic_combine(hash, &definition->has_depth_attachment,       sizeof(bool));
	hash = hash_bytes_generic_combine(hash, &definition->samples,                    sizeof(VkSampleCountFlagBits));
	hash = hash_bytes_generic_combine(hash, &definition->min_sample_shading_enabled, sizeof(bool));
	hash = hash_bytes_generic_combine(hash, &definition->min_sample_shading,         sizeof(float));
	hash = hash_bytes_generic_combine(hash, &definition->view_mask,                  sizeof(u32));

	VkPipeline *fetched_pipeline = hash_table_fetch(&device->pipeline_cache, hash);

	if (fetched_pipeline) {
		struct gfx_pipeline_st st = {0};
		st.pipeline = *fetched_pipeline;
		st.layout = layout;
		st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

		return st;
	}

	struct gfx_pipeline_st st = {0};
	st.pipeline = gfx_device_pipeline_create_graphics(device, layout, definition);
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	hash_table_add(&device->pipeline_cache, hash, &st.pipeline);

	return st;
}

struct gfx_pipeline_st gfx_device_pipeline_fetch_compute(struct gfx_device *device, struct gfx_compute_pipeline_def *definition)
{
	VkPipelineLayout layout = gfx_device_pipeline_layout_fetch(device, definition->program);

	u64 hash = 0;
	hash = hash_bytes_generic_combine(hash, definition->program, sizeof(struct gfx_shader_program));

	VkPipeline *fetched_pipeline = hash_table_fetch(&device->pipeline_cache, hash);

	if (fetched_pipeline) {
		struct gfx_pipeline_st st = {0};
		st.pipeline = *fetched_pipeline;
		st.layout = layout;
		st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

		return st;
	}

	struct gfx_pipeline_st st = {0};
	st.pipeline = gfx_device_pipeline_create_compute(device, layout, definition);
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
	
	hash_table_add(&device->pipeline_cache, hash, &st.pipeline);

	return st;
}

struct gfx_sampler gfx_device_sampler_create_ext(struct gfx_device *device,
						 VkFilter filter,
						 VkSamplerAddressMode wrap_x,
						 VkSamplerAddressMode wrap_y,
						 VkSamplerAddressMode wrap_z,
						 VkBorderColor border_colour)
{
	VkPhysicalDeviceProperties properties =	device->physical_device_properties.properties;

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

	struct gfx_sampler sampler = {0};
	sampler.filter = filter;
	sampler.wrap_x = wrap_x;
	sampler.wrap_y = wrap_y;
	sampler.wrap_z = wrap_z;
	sampler.border_colour = border_colour;

	GFX_VK_CHECK(vkCreateSampler(device->device,
				     &create_info, NULL,
				     &sampler.handle),
		     "Failed to create texture sampler.");

	sampler.bindless = gfx_bindless_register_sampler(&device->bindless, sampler.handle);

	return sampler;
}

struct gfx_sampler gfx_device_sampler_create(struct gfx_device *device, VkFilter filter)
{
	return gfx_device_sampler_create_ext(device,
					     filter,
					     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
					     VK_BORDER_COLOR_INT_OPAQUE_BLACK);
}

void gfx_device_sampler_destroy(struct gfx_device *device, struct gfx_sampler *sampler)
{
	vkDestroySampler(device->device, sampler->handle, NULL);
	sampler->handle = VK_NULL_HANDLE;
}

static u32 clamp_mimap_count(u32 mipmaps, u32 w, u32 h, u32 d)
{
	return min_value(mipmaps, 1u + (u32)log2f((float)max_value(w, max_value(h, d))));
}

struct gfx_texture gfx_device_texture_alloc(struct gfx_device *device,
					    u32 width, u32 height, u32 depth,
					    VkFormat format,
					    VkImageViewType type,
					    VkImageTiling tiling,
					    u32 mipmaps,
					    VkSampleCountFlagBits samples,
					    bool is_transient, bool is_storage)
{
	struct gfx_texture texture = {0};

	texture.width = width;
	texture.height = height;
	texture.depth = depth;

	texture.format = format;
	texture.type = type;
	texture.tiling = tiling;

	texture.mipmap_count = clamp_mimap_count(mipmaps, width, height, depth);
	texture.samples = samples;

	if (is_transient)
		texture.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	else
		texture.usage =
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	texture.is_depth = format == device->depth_format;
	texture.is_cubemap = type == VK_IMAGE_VIEW_TYPE_CUBE;
	texture.is_storage = is_storage;
	texture.is_swapchain = false;
	
	if (is_storage)
		texture.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (texture.is_depth)
		texture.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	else
		texture.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
	VkImageType image_type = VK_IMAGE_TYPE_MAX_ENUM;

	switch (texture.type) {
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
		debug_log_crash("Failed to find VkImageType given VkImageViewType: %d", texture.type);
		break;
	}

	texture.aspect_flags = texture.is_depth
		? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
		: VK_IMAGE_ASPECT_COLOR_BIT;

	texture.aspect_count = 0;
	
	for (VkImageAspectFlags b = 1; b <= texture.aspect_flags; b <<= 1) {
		if (texture.aspect_flags & b)
			texture.aspect_count++;
	}

	texture.access_count = texture.aspect_count * texture.mipmap_count * gfx_texture_layer_count(&texture);

	texture.access_types = memory_arena_array(device->arena,
						  texture.access_count,
						  sizeof(enum gfx_texture_access_type));
	
	VkImageCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = image_type;
	create_info.extent.width = texture.width;
	create_info.extent.height = texture.height;
	create_info.extent.depth = texture.depth;
	create_info.mipLevels = texture.mipmap_count;
	create_info.arrayLayers = gfx_texture_layer_count(&texture);
	create_info.format = texture.format;
	create_info.tiling = texture.tiling;
	create_info.usage = texture.usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = texture.samples;
	create_info.flags = 0;

	if (texture.is_cubemap)
		create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	vma_alloc_info.priority = 1.f;

	GFX_VK_CHECK(vmaCreateImage(device->vma_allocator, &create_info,
				    &vma_alloc_info, &texture.handle,
				    &texture.allocation, &texture.allocation_info),
		     "Failed to allocate texture.");
	
	return texture;
}

struct gfx_texture gfx_device_texture_alloc_2d(struct gfx_device *device,
					       u32 width, u32 height,
					       VkFormat format,
					       u32 mipmaps)
{
	return gfx_device_texture_alloc(device,
					width, height, 1,
					format,
					VK_IMAGE_VIEW_TYPE_2D,
					VK_IMAGE_TILING_OPTIMAL,
					mipmaps,
					VK_SAMPLE_COUNT_1_BIT,
					false, false);
}

struct gfx_texture gfx_device_texture_alloc_2d_rw(struct gfx_device *device,
						  u32 width, u32 height,
						  VkFormat format,
						  u32 mipmaps)
{
	return gfx_device_texture_alloc(device,
					width, height, 1,
					format,
					VK_IMAGE_VIEW_TYPE_2D,
					VK_IMAGE_TILING_OPTIMAL,
					mipmaps,
					VK_SAMPLE_COUNT_1_BIT,
					false, true);
}

struct gfx_texture gfx_device_texture_alloc_depth_2d(struct gfx_device *device,
						     u32 width, u32 height,
						     u32 mipmaps)
{
	return gfx_device_texture_alloc_2d(device, width, height, device->depth_format, mipmaps);
}

struct gfx_texture gfx_device_texture_alloc_depth_2d_rw(struct gfx_device *device,
							u32 width, u32 height,
							u32 mipmaps)
{
	return gfx_device_texture_alloc_2d_rw(device, width, height, device->depth_format, mipmaps);
}

struct gfx_texture gfx_device_texture_alloc_cubemap(struct gfx_device *device,
						    u32 resolution,
						    VkFormat format,
						    u32 mipmaps)
{
	return gfx_device_texture_alloc(device,
					resolution, resolution, 1,
					format,
					VK_IMAGE_VIEW_TYPE_CUBE,
					VK_IMAGE_TILING_OPTIMAL,
					mipmaps,
					VK_SAMPLE_COUNT_1_BIT,
					false, false);
}


struct gfx_texture gfx_device_texture_alloc_cubemap_depth(struct gfx_device *device,
							  u32 resolution,
							  VkFormat format,
							  u32 mipmaps)
{
	return gfx_device_texture_alloc(device,
					resolution, resolution, 1,
					device->depth_format,
					VK_IMAGE_VIEW_TYPE_CUBE,
					VK_IMAGE_TILING_OPTIMAL,
					mipmaps,
					VK_SAMPLE_COUNT_1_BIT,
					false, false);
}

void gfx_device_texture_destroy(struct gfx_device *device, struct gfx_texture *texture)
{
	vmaDestroyImage(device->vma_allocator, texture->handle, texture->allocation);
}

struct gfx_texture_view gfx_device_texture_view_create(struct gfx_device *device,
						       struct gfx_texture *texture,
						       u32 layer_count,
						       u32 base_layer,
						       u32 base_level)
{
	VkImageViewType view_type = texture->type;

	if (texture->is_cubemap && layer_count == 1)
		view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

	VkImageAspectFlags aspect = texture->is_depth
		? VK_IMAGE_ASPECT_DEPTH_BIT
		: VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = texture->handle;
	view_create_info.viewType = view_type;
	view_create_info.format = texture->format;

	u32 level_count = texture->mipmap_count - base_level;

	view_create_info.subresourceRange.aspectMask = aspect;
	view_create_info.subresourceRange.baseMipLevel = base_level;
	view_create_info.subresourceRange.levelCount = level_count;
	view_create_info.subresourceRange.baseArrayLayer = base_layer;
	view_create_info.subresourceRange.layerCount = layer_count;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	struct gfx_texture_view view = {0};
	view.parent = texture;
	view.base_layer = base_layer;
	view.layer_count = layer_count;
	view.base_level = base_level;
	view.level_count = level_count;
	view.aspect = aspect;
	
	GFX_VK_CHECK(vkCreateImageView(device->device,
				       &view_create_info, NULL,
				       &view.handle),
		     "Failed to create texture image view.");

	// Swapchain images are omitted from being accessible bindlessly.
	if (!texture->is_swapchain)
		view.bindless = gfx_bindless_register_view(&device->bindless, view.handle,
							   true, texture->is_storage);
	
	return view;
}

void gfx_device_texture_view_destroy(struct gfx_device *device, struct gfx_texture_view *view)
{
	vkDestroyImageView(device->device, view->handle, NULL);
}

struct gfx_texture_view *gfx_device_texture_view_fetch(struct gfx_device *device,
						       struct gfx_texture *texture,
						       u32 layer_count,
						       u32 base_layer,
						       u32 base_level)
{
	u64 hash = 0;
	hash = hash_bytes_generic_combine(hash,  texture,        sizeof(struct gfx_texture)); // TODO: Hash with a gfx_texture_hash(...) function that doesn't just hash the whole image incl. vulkan handle?
	hash = hash_bytes_generic_combine(hash, &layer_count,    sizeof(u32));
	hash = hash_bytes_generic_combine(hash, &base_layer,     sizeof(u32));
	hash = hash_bytes_generic_combine(hash, &base_level,     sizeof(u32));

	struct gfx_texture_view *fetched_image_view = hash_table_fetch(&device->texture_view_cache, hash);

	if (fetched_image_view)
		return fetched_image_view;

	struct gfx_texture_view view = gfx_device_texture_view_create(device,
								      texture,
								      layer_count,
								      base_layer,
								      base_level);

	return hash_table_add(&device->texture_view_cache, hash, &view);
}

struct gfx_texture_view *gfx_device_texture_view_fetch_std(struct gfx_device *device,
							   struct gfx_texture *texture)
{
	return gfx_device_texture_view_fetch(device, texture, gfx_texture_layer_count(texture), 0, 0);
}

struct gfx_buffer gfx_device_buffer_alloc(struct gfx_device *device,
					  VkBufferUsageFlags2 usage,
					  VmaAllocationCreateFlagBits flags,
					  u64 size)
{
	struct gfx_buffer buffer = {0};
	buffer.usage = usage;
	buffer.size = size;
	buffer.allocator = &device->vma_allocator;
	buffer.allocation_flags = flags;
	
	if (gfx_buffer_is_storage(&buffer))
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
	vma_alloc_info.flags = flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	GFX_VK_CHECK(vmaCreateBuffer(device->vma_allocator,
				     &buffer_create_info,
				     &vma_alloc_info,
				     &buffer.handle,
				     &buffer.allocation,
				     &buffer.allocation_info),
		     "Failed to allocate buffer.");

	if (gfx_buffer_is_storage(&buffer)) {
		VkBufferDeviceAddressInfo address_info = {0};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer.handle;
		buffer.device_address = vkGetBufferDeviceAddress(device->device, &address_info);
	}

	return buffer;
}

void gfx_device_buffer_destroy(struct gfx_device *device, struct gfx_buffer *buffer)
{
	vmaDestroyBuffer(device->vma_allocator, buffer->handle, buffer->allocation);
}

// TODO: Move elsewhere.
struct string8 gfx_load_file_bytes(struct memory_arena *dst, struct string8 path)
{
	b8 *bytes = NULL;

	FILE *file = fopen((char *)path.str, "rb");
	u64 file_size = 0;

	if (file) {
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		bytes = memory_arena_push(dst, file_size);
		fread(bytes, file_size, 1, file);

		fclose(file);
	}

	return string8_init(bytes, file_size);
}

struct gfx_shader_stage gfx_device_shader_stage_load_from_bytecode(struct gfx_device *device,
								   struct string8 path)
{
	struct scratch_arena scratch = scratch_begin(NULL, 0);

	struct string8 source = gfx_load_file_bytes(scratch.arena, path);
	
	SpvReflectShaderModule reflect_module = {0};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(source.len, source.str, &reflect_module);

	if (reflect_result != SPV_REFLECT_RESULT_SUCCESS)
		debug_log_crash("Failed to reflect SPIR-V module: %d\n", reflect_result);

	struct gfx_shader_stage stage = {0};

	if (reflect_module.entry_point_count >= 1) {
		stage.stage = (VkShaderStageFlags)reflect_module.entry_points[0].shader_stage;
	} else {
		debug_log_crash("No entry points found in SPIR-V.\n");
		goto exit;
	}

	u32 push_constant_count = 0;
	reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_module,
							       &push_constant_count, NULL);

	if (reflect_result == SPV_REFLECT_RESULT_SUCCESS && push_constant_count > 0) {
		SpvReflectBlockVariable **pcs = memory_arena_array(scratch.arena,
								   push_constant_count,
								   sizeof(SpvReflectBlockVariable *));

		spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, pcs);

		for (u32 i = 0; i < push_constant_count; i++) {
			SpvReflectBlockVariable *pc = pcs[i];

			u32 alignment = 4;
				
			for (u32 j = 0; j < pc->member_count; j++)
				alignment = max_value(alignment, pc->members[j].size);

			u32 padded = MEMORY_ALIGN_UP(pc->size, alignment);
			stage.push_constant_size = max_value(stage.push_constant_size, padded);
		}
	}

	VkShaderModuleCreateInfo module_create_info = {0};
	module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_create_info.codeSize = source.len;
	module_create_info.pCode = (const u32 *)source.str;

	GFX_VK_CHECK(vkCreateShaderModule(device->device,
					  &module_create_info, NULL,
					  &stage.module),
		     "Failed to create shader module.");

exit:
	spvReflectDestroyShaderModule(&reflect_module);
	scratch_release(&scratch);

	return stage;
}

void gfx_device_shader_stage_destroy(struct gfx_device *device, struct gfx_shader_stage *stage)
{
	vkDestroyShaderModule(device->device, stage->module, NULL);
}

struct gfx_shader_program gfx_device_shader_program_create(struct gfx_device *device,
							   u32 stage_count,
							   struct string8 *stage_paths)
{
	struct gfx_shader_program program = {0};
	program.stage_count = stage_count;

	for (u32 i = 0; i < stage_count; i++) {
		printf("Loading shader stage: %.*s\n", (u32)stage_paths[i].len, stage_paths[i].str);
		program.stages[i] = gfx_device_shader_stage_load_from_bytecode(device, stage_paths[i]);
		program.push_constant_size = max_value(program.push_constant_size, program.stages[i].push_constant_size);
	}
	
	return program;
}

void gfx_device_shader_program_destroy(struct gfx_device *device, struct gfx_shader_program *program)
{
	for (int i = 0; i < program->stage_count; i++)
		gfx_device_shader_stage_destroy(device, program->stages + i);
}

struct gfx_command_pool gfx_device_command_pool_create(struct gfx_device *device, u32 family_index)
{
	struct gfx_command_pool pool = {0};
	
	VkCommandPoolCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;
	
	GFX_VK_CHECK(vkCreateCommandPool(device->device,
					 &create_info, NULL,
					 &pool.handle),
		     "Failed to create command pool.");

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {0};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = array_size(pool.free_buffers);
	command_buffer_allocate_info.commandPool = pool.handle;

	GFX_VK_CHECK(vkAllocateCommandBuffers(device->device,
					      &command_buffer_allocate_info,
					      pool.free_buffers),
		     "Failed to allocate command pool command buffers.");

	return pool;
}

void gfx_device_command_pool_destroy(struct gfx_device *device, struct gfx_command_pool *pool)
{
	vkDestroyCommandPool(device->device, pool->handle, NULL);
}

void gfx_device_command_pool_reset(struct gfx_device *device, struct gfx_command_pool *pool)
{
	pool->free_index = 0;
	vkResetCommandPool(device->device, pool->handle, 0);
}
