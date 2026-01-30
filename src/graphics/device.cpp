#include "device.h"

#include "ext/spirv_reflect.h"

#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_vulkan.h"

#include "core/scratch.h"
#include "core/hash.h"

#include "platform/platform.h"

#include "math/calc.h"

using namespace gfx;

static const char *graphics_validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};

static VkFormat find_graphics_supported_format(
	VkPhysicalDevice physical_device,
	VkImageTiling tiling,
	VkFormatFeatureFlags features,
	u32 candidate_count,
	VkFormat *candidates
)
{
	for (int i = 0; i < candidate_count; i++) {
		VkFormatProperties properties = {};
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

	return find_graphics_supported_format(
		physical_device,
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
		array_size(candidates), candidates
	);
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

static const char *const *get_instance_extensions(MemoryArena &arena, u32 *extension_count)
{
	const char *const *names = platform::get_vulkan_instance_extensions(extension_count);

	if (!names)
		debug_log_crash("Unable to get instance extension count.");

	u32 extra_extension_count = 3;

#ifdef __APPLE__
	extra_extension_count += 2;
#endif

	const char **extensions = arena.push_array<const char *>(*extension_count + extra_extension_count);

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

static bool check_graphics_physical_device_extension_support(VkPhysicalDevice physical_device)
{
	u32 extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
					     &extension_count, nullptr);

	if (extension_count <= 0)
		debug_log_crash("Failed to find any device extension properties.");

	ScratchArena scratch;

	VkExtensionProperties *available_exts = scratch.get_arena().push_array<VkExtensionProperties>(extension_count);

	vkEnumerateDeviceExtensionProperties(
		physical_device, nullptr,
		&extension_count, available_exts
	);

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
	return result;
}

static bool check_for_validation_layer_support(MemoryArena &arena)
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);

	ScratchArena scratch;

	VkLayerProperties *available_layers = scratch.get_arena().push_array<VkLayerProperties>(layer_count);
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
	return result;
}

static SwapchainSupportDetails query_swapchain_support(
	VkPhysicalDevice physical_device,
	VkSurfaceKHR surface
)
{
	SwapchainSupportDetails result = {};

	u32 surface_format_count = 0;
	u32 present_mode_count = 0;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &result.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, nullptr);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

	if (surface_format_count > 0) {
		result.surface_formats.resize(surface_format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physical_device, surface,
			&surface_format_count,
			result.surface_formats.data()
		);
	}

	if (present_mode_count > 0) {
		result.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			physical_device, surface,
			&present_mode_count,
			result.present_modes.data()
		);
	}

	return result;
}

static u32 assign_graphics_physical_device_usability(
	VkSurfaceKHR surface,
	VkPhysicalDevice physical_device,
	VkPhysicalDeviceProperties2 properties,
	VkPhysicalDeviceFeatures2 features,
	bool *has_essentials
)
{
	u32 usability = 0;

	bool adequate_swap_chain = false;
	bool has_required_extensions = check_graphics_physical_device_extension_support(physical_device);
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
		SwapchainSupportDetails details = query_swapchain_support(physical_device, surface);

		adequate_swap_chain =
			(details.surface_formats.size() > 0) &&
			(details.present_modes.size() > 0);

		usability += 3;
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

static VKAPI_ATTR VkBool32 VKAPI_CALL graphics_vulkan_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data)
{
	if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		debug_log_crash(
			"Severity = %d, Type = %d, Message = \"%s\"",
			message_severity, message_type, callback_data->pMessage
		);
	}

	return VK_FALSE;
}

static VkResult create_graphics_device_debug_utils_messenger_ext(
	VkInstance instance,
	VkDebugUtilsMessengerCreateInfoEXT *debug_info,
	const VkAllocationCallbacks *allocator,
	VkDebugUtilsMessengerEXT *messenger
)
{
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn)
		return fn(instance, debug_info, allocator, messenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static VkSurfaceFormatKHR _choose_swapchain_surface_format(const Vector<VkSurfaceFormatKHR> &available_surface_formats)
{
	for (auto &format : available_surface_formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
		    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			debug_log("Found desired swapchain swap surface format and colour space.");
			return format;
		}
	}

	debug_log("Could not find desired swapchain swap surface format and colour space, falling back...");

	return available_surface_formats[0];
}

static VkPresentModeKHR _choose_swapchain_present_mode(const Vector<VkPresentModeKHR> &available_present_modes, bool enable_vsync)
{
	if (!enable_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for (auto &mode : available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D _choose_swapchain_extent(const VkSurfaceCapabilitiesKHR *capabilities)
{
	if (capabilities->currentExtent.width != -1u &&
	    capabilities->currentExtent.height != -1u)
		return capabilities->currentExtent;

	int window_width, window_height;
	platform::get_window_size(&window_width, &window_height);

	VkExtent2D actual_extent = {
		(u32)window_width,
		(u32)window_height
	};

	actual_extent.width = CalcU::clamp(
		actual_extent.width,
		capabilities->minImageExtent.width,
		capabilities->maxImageExtent.width
	);

	actual_extent.height = CalcU::clamp(
		actual_extent.height,
		capabilities->minImageExtent.height,
		capabilities->maxImageExtent.height
	);

	return actual_extent;
}

Device::Device()
	: instance()
	, device()
	, physical_device()
	, physical_device_properties()
	, physical_device_features()
	, vma_allocator()
	, surface()
	, pipeline_process_cache()
	, debug_messenger()
	, has_validation_layers()
	, current_frame_index()
	, graphics_queue()
	, per_frame_data{}
	, depth_format()
	, max_msaa_samples()
	, swapchain_details()
	, bindless()
	, texture_view_cache()
	, pipeline_cache()
	, pipeline_layout_cache()
	, imgui_pool(VK_NULL_HANDLE)
{
}

Device::~Device()
{
}

void Device::init()
{
	VkApplicationInfo core_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = DEFAULT_WINDOW_TITLE,
		.applicationVersion = VK_MAKE_API_VERSION(
			APP_VERSION_VARIANT,
			APP_VERSION_MAJOR,
			APP_VERSION_MINOR,
			APP_VERSION_PATCH
		),
		.pEngineName = ENGINE_NAME,
		.engineVersion = VK_MAKE_API_VERSION(
			ENGINE_VERSION_VARIANT,
			ENGINE_VERSION_MAJOR,
			ENGINE_VERSION_MINOR,
			ENGINE_VERSION_PATCH
		),
		.apiVersion = VK_API_VERSION_1_4
	};

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &core_info;

	volkInitialize();

	ScratchArena scratch;

	instance_create_info.ppEnabledExtensionNames = get_instance_extensions(scratch.get_arena(), &instance_create_info.enabledExtensionCount);

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
	debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	debug_create_info.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	debug_create_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	debug_create_info.pfnUserCallback = graphics_vulkan_debug_callback;
	debug_create_info.pUserData = nullptr;

	has_validation_layers = check_for_validation_layer_support(scratch.get_arena());

	if (has_validation_layers) {
		debug_log("Validation layer support verified.");

		instance_create_info.enabledLayerCount = array_size(graphics_validation_layers);
		instance_create_info.ppEnabledLayerNames = graphics_validation_layers;
		instance_create_info.pNext = &debug_create_info;
	} else {
		debug_log("No validation layer support.");

		instance_create_info.enabledLayerCount = 0;
		instance_create_info.ppEnabledLayerNames = nullptr;
		instance_create_info.pNext = nullptr;
	}

#ifdef __APPLE__
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	GFX_VK_CHECK(
		vkCreateInstance(&instance_create_info, nullptr, &instance),
		"Failed to create instance."
	);

	volkLoadInstance(instance);

	if (has_validation_layers) {
		GFX_VK_CHECK(
			create_graphics_device_debug_utils_messenger_ext(
				instance,
				&debug_create_info, nullptr,
				&debug_messenger
			),
			"Failed to create debug messenger."
		);
	}

	if (!platform::create_vulkan_surface(instance, &surface))
		debug_log_crash("Failed to create surface.");

	// Enumerate physical_resource devices.
	{
		u32 device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

		if (device_count <= 0)
			debug_log_crash("Failed to find GPUs with Vulkan support.");

		VkPhysicalDeviceProperties2 properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		VkPhysicalDeviceFeatures2   features   = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

		VkPhysicalDevice *devices = scratch.get_arena().push_array<VkPhysicalDevice>(device_count);

		vkEnumeratePhysicalDevices(instance, &device_count, devices);

		u32 best_usability = 0;
		u32 selected_id = 0;

		for (int i = 0; i < device_count; i++) {
			vkGetPhysicalDeviceProperties2(devices[i], &properties);
			vkGetPhysicalDeviceFeatures2(devices[i], &features);

			debug_log(
				"Querying physical device: %s (%d)",
				properties.properties.deviceName, properties.properties.deviceID
			);

			bool has_essentials = false;

			u32 current_usability = assign_graphics_physical_device_usability(
				surface,
				devices[i],
				properties, features,
				&has_essentials
			);

			if (current_usability > best_usability && has_essentials) {
				physical_device = devices[i];
				physical_device_properties = properties;
				physical_device_features = features;

				best_usability = current_usability;
				selected_id = properties.properties.deviceID;
			}
		}

		if (!physical_device)
			debug_log_crash("Unable to find a suitable GPU.");

		debug_log("Selected a suitable GPU: %d", selected_id);
	}

	max_msaa_samples = find_graphics_max_usable_sample_count(physical_device_properties);
	depth_format = find_graphics_depth_format(physical_device);

	// Locate the graphics queue.
	// TODO: move this start stuff into the queue class!!!!!
	{
		graphics_queue.device = this;

		u32 queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);

		if (queue_family_count <= 0)
			debug_log_crash("Failed to find any queue families.");

		VkQueueFamilyProperties *queue_families = scratch.get_arena().push_array<VkQueueFamilyProperties>(queue_family_count);

		vkGetPhysicalDeviceQueueFamilyProperties(
			physical_device,
			&queue_family_count,
			queue_families
		);

		for (int i = 0; i < queue_family_count; i++) {
			if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
				VkBool32 present_support = VK_FALSE;

				vkGetPhysicalDeviceSurfaceSupportKHR(
					physical_device, i,
					surface,
					&present_support
				);

				if (present_support) {
					graphics_queue.family_index = i;
					break;
				}

				continue;
			}
		}
	}

	float queue_priority = 1.f;

	VkDeviceQueueCreateInfo graphics_queue_create_info = {};
	graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphics_queue_create_info.queueFamilyIndex = graphics_queue.family_index;
	graphics_queue_create_info.queueCount = 1;
	graphics_queue_create_info.pQueuePriorities = &queue_priority;

	// Disable this so we get a clear indication if something's gone wrong.
	physical_device_features.features.robustBufferAccess = VK_FALSE;

	VkPhysicalDeviceVulkan11Features vulkan11_features = {};
	vulkan11_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	vulkan11_features.shaderDrawParameters = VK_TRUE;
	vulkan11_features.multiview = VK_TRUE;

	VkPhysicalDeviceVulkan12Features vulkan12_features = {};
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
	vulkan12_features.pNext = &vulkan11_features;

	VkPhysicalDeviceVulkan13Features vulkan13_features = {};
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

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = nullptr;
	device_create_info.enabledExtensionCount = array_size(device_extensions);
	device_create_info.ppEnabledExtensionNames = device_extensions;
	device_create_info.pEnabledFeatures = &physical_device_features.features;
	device_create_info.pNext = &vulkan13_features;

	if (has_validation_layers) {
		device_create_info.enabledLayerCount = array_size(graphics_validation_layers);
		device_create_info.ppEnabledLayerNames = graphics_validation_layers;
		debug_log("Enabled validation layers.");
	}

	GFX_VK_CHECK(
		vkCreateDevice(physical_device,
			&device_create_info, nullptr,
			&device
		),
		"Failed to create logical device."
	);

	vkGetDeviceQueue(
		device,
		graphics_queue.family_index, 0,
		&graphics_queue.handle
	);

	debug_log("Created logical device.");
	
	// TODO: MOVE THIS INTO THE QUEUE CLASS!!!!
	graphics_queue.timeline_value = 0;

	VkSemaphoreTypeCreateInfo timeline_type_create_info = {};
	timeline_type_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	timeline_type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timeline_type_create_info.initialValue = graphics_queue.timeline_value;

	VkSemaphoreCreateInfo timeline_semaphore_create_info = {};
	timeline_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	timeline_semaphore_create_info.flags = 0;
	timeline_semaphore_create_info.pNext = &timeline_type_create_info;
		
	GFX_VK_CHECK(
		vkCreateSemaphore(
			device,
			&timeline_semaphore_create_info, nullptr,
			&graphics_queue.timeline_semaphore
		),
		"Failed to create timeline semaphore for queue."
	);
	
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
		graphics_queue.frames[i].command_pool = create_command_pool(graphics_queue.family_index);
	
	debug_log("Initialised graphics queue.");

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkFenceCreateInfo in_flight_fence_create_info = {};
		in_flight_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		in_flight_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		per_frame_data[i].expected_timeline_value = 0;

		VkSemaphoreCreateInfo binary_semaphore_create_info = {};
		binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		binary_semaphore_create_info.flags = 0;
		binary_semaphore_create_info.pNext = nullptr;

		GFX_VK_CHECK(
			vkCreateSemaphore(
				device,
				&binary_semaphore_create_info, nullptr,
				&per_frame_data[i].image_available_semaphore
			),
			"Failed to create image available semaphore."
		);

		GFX_VK_CHECK(
			vkCreateSemaphore(
				device,
				&binary_semaphore_create_info, nullptr,
				&per_frame_data[i].render_finished_semaphore
			),
			"Failed to create render finished semaphore."
		);
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

	volkLoadDevice(device);

	VmaVulkanFunctions vulkan_functions = {};
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

	VmaAllocatorCreateInfo allocator_create_info = {};
	allocator_create_info.physicalDevice = physical_device;
	allocator_create_info.device = device;
	allocator_create_info.instance = instance;
	allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	//vmaImportVulkanFunctionsFromVolk(&allocator_create_info, &vulkan_functions);

	allocator_create_info.pVulkanFunctions = &vulkan_functions;

	GFX_VK_CHECK(vmaCreateAllocator(&allocator_create_info, &vma_allocator),
		     "Failed to create Vulkan Memory Allocator.");

	debug_log("Created Vulkan Memory Allocator.");

	VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = nullptr;
	pipeline_cache_create_info.flags = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = nullptr;

	GFX_VK_CHECK(
		vkCreatePipelineCache(
			device,
			&pipeline_cache_create_info, nullptr,
			&pipeline_process_cache
		),
		"Failed to process pipeline cache."
	);

	debug_log("Created graphics pipeline process cache.");

	create_bindless();

	swapchain_details = query_swapchain_support(physical_device, surface);
}

void Device::destroy()
{
	for (auto &[key, view] : texture_view_cache)
		destroy_texture_view(view);

	for (auto &[key, pipeline] : pipeline_cache)
		destroy_pipeline(pipeline);

	for (auto &[key, layout] : pipeline_layout_cache)
		destroy_pipeline_layout(layout);

	texture_view_cache.clear();
	pipeline_cache.clear();
	pipeline_layout_cache.clear();

	destroy_bindless();
	
	graphics_queue.destroy();

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		destroy_semaphore(per_frame_data[i].image_available_semaphore);
		destroy_semaphore(per_frame_data[i].render_finished_semaphore);
	}

	destroy_imgui();

	vkDestroyPipelineCache(device, pipeline_process_cache, nullptr);
	platform::destroy_vulkan_surface(instance, surface);
	vmaDestroyAllocator(vma_allocator);
	vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
	vkDestroyDevice(device, nullptr);
}

void Device::wait_idle()
{
	vkDeviceWaitIdle(device);
}

void Device::wait_for_fence(VkFence fence)
{
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void Device::reset_fence(VkFence fence)
{
	vkResetFences(device, 1, &fence);
}

void Device::destroy_fence(VkFence fence)
{
	vkDestroyFence(device, fence, nullptr);
}

void Device::destroy_semaphore(VkSemaphore semaphore)
{
	vkDestroySemaphore(device, semaphore, nullptr);
}

CommandBuffer Device::begin_frame(Swapchain &swapchain)
{
	PerFrameData &frame_data = per_frame_data[current_frame_index];

	if (frame_data.expected_timeline_value > 0)
		graphics_queue.wait_until(frame_data.expected_timeline_value);

	VkAcquireNextImageInfoKHR acquire_next_image_info = {};
	acquire_next_image_info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquire_next_image_info.swapchain = swapchain.get_handle();
	acquire_next_image_info.timeout = UINT64_MAX;
	acquire_next_image_info.semaphore = frame_data.image_available_semaphore;
	acquire_next_image_info.fence = VK_NULL_HANDLE;
	acquire_next_image_info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(device, &acquire_next_image_info, &swapchain.current_texture_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		debug_log_crash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		debug_log_crash("Failed to acquire next image in swapchain.");

	graphics_queue.reset_pool();

	return graphics_queue.get_command_buffer();
}

void Device::end_frame(const Swapchain &swapchain, CommandBuffer &cmd)
{
	PerFrameData &frame_data = per_frame_data[current_frame_index];

	apply_bindless_updates();

	VkSemaphoreSubmitInfo image_available_semaphore = {};
	image_available_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore.semaphore = frame_data.image_available_semaphore;
	image_available_semaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSemaphoreSubmitInfo render_finished_semaphore = {};
	render_finished_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore.semaphore = frame_data.render_finished_semaphore;
	render_finished_semaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	frame_data.expected_timeline_value = graphics_queue.submit(
		cmd,
		{ image_available_semaphore },
		{ render_finished_semaphore },
		VK_NULL_HANDLE
	);

	graphics_queue.present(swapchain, { render_finished_semaphore.semaphore });

	current_frame_index = (current_frame_index + 1) % FRAMES_IN_FLIGHT;
}

Queue &Device::graphics()
{
	return graphics_queue;
}

CommandPool Device::create_command_pool(u32 family_index)
{
	VkCommandPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;
	
	CommandPool pool;

	GFX_VK_CHECK(
		vkCreateCommandPool(
			device,
			&create_info, nullptr,
			&pool.handle
		),
		"Failed to create command pool."
	);

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = array_size(pool.free_buffers);
	command_buffer_allocate_info.commandPool = pool.handle;

	GFX_VK_CHECK(
		vkAllocateCommandBuffers(
			device,
			&command_buffer_allocate_info,
			pool.free_buffers
		),
		"Failed to allocate command pool command buffers."
	);

	return pool;
}

void Device::destroy_command_pool(const CommandPool &pool)
{
	vkDestroyCommandPool(device, pool.get_handle(), nullptr);
}

void Device::reset_command_pool(CommandPool &pool)
{
	pool.free_index = 0;
	vkResetCommandPool(device, pool.get_handle(), 0);
}

Swapchain Device::create_swapchain()
{
	ScratchArena scratch;

	SwapchainSupportDetails details = swapchain_details;

	VkSurfaceFormatKHR surface_format = _choose_swapchain_surface_format(details.surface_formats);
	VkPresentModeKHR present_mode = _choose_swapchain_present_mode(details.present_modes, true);
	VkExtent2D extent = _choose_swapchain_extent(&details.capabilities);

	Swapchain swapchain;

	swapchain.width = extent.width;
	swapchain.height = extent.height;
	swapchain.format = surface_format.format;

	u32 texture_count = details.capabilities.minImageCount + 1;

	if (details.capabilities.maxImageCount > 0 && texture_count > details.capabilities.maxImageCount)
		texture_count = details.capabilities.maxImageCount;

	constexpr VkImageUsageFlags swapchain_texture_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = texture_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = swapchain_texture_usage;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = nullptr;
	create_info.preTransform = details.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreateSwapchainKHR(device,
			&create_info, nullptr,
			&swapchain.handle
		),
		"Failed to create swapchain."
	);

	vkGetSwapchainImagesKHR(device, swapchain.handle, &texture_count, nullptr);

	if (texture_count <= 0)
		debug_log_crash("Failed to find any images in swapchain.");

	VkImage *vk_images = scratch.get_arena().push_array<VkImage>(texture_count);

	vkGetSwapchainImagesKHR(device, swapchain.handle, &texture_count, vk_images);

	swapchain.textures.resize(texture_count);
	swapchain.views.resize(texture_count);

	for (int i = 0; i < texture_count; i++) {
		Texture &texture = swapchain.textures[i];

		texture.handle = vk_images[i];

		texture.width = swapchain.width;
		texture.height = swapchain.height;
		texture.depth = 1;

		texture.is_depth_texture     = false;
		texture.is_cubemap_texture   = false;
		texture.is_storage_texture   = false;
		texture.is_swapchain_texture = true;

		texture.format = swapchain.format;
		texture.type   = VK_IMAGE_TYPE_2D;
		texture.tiling = VK_IMAGE_TILING_OPTIMAL;

		texture.usage = swapchain_texture_usage;

		texture.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
		texture.aspect_count = 1;

		texture.layer_count = 1;
		texture.mipmap_count = 1;
		texture.sample_count = VK_SAMPLE_COUNT_1_BIT;

		SubresourceRange range = {};
		range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
		range.base_mip = 0;
		range.mips = 1;
		range.base_layer = 0;
		range.layers = 1;

		swapchain.views[i] = create_texture_view(&texture, VK_IMAGE_VIEW_TYPE_2D, range);
	}

	debug_log("Swapchain created.");

	return swapchain;
}

void Device::destroy_swapchain(const Swapchain &swapchain)
{
	for (int i = 0; i < swapchain.views.size(); i++)
		destroy_texture_view(swapchain.views[i]);

	vkDestroySwapchainKHR(device, swapchain.get_handle(), nullptr);
}

VkPipelineLayout Device::create_pipeline_layout(const ShaderProgram *program)
{
	VkShaderStageFlags stage = program->is_compute()
		? VK_SHADER_STAGE_COMPUTE_BIT
		: VK_SHADER_STAGE_ALL_GRAPHICS;

	VkPushConstantRange push_constants = {};
	push_constants.offset = 0;
	push_constants.size = program->get_push_constant_size();
	push_constants.stageFlags = stage;

	VkPipelineLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = BINDLESS_SET_MAX_ENUM;
	create_info.pSetLayouts = bindless.get_layouts();

	if (push_constants.size > 0) {
		create_info.pushConstantRangeCount = 1;
		create_info.pPushConstantRanges = &push_constants;
	} else {
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = nullptr;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreatePipelineLayout(
			device,
			&create_info, nullptr,
			&layout
		),
		"Failed to create pipeline layout."
	);

	return layout;
}

VkPipelineLayout Device::fetch_pipeline_layout(const ShaderProgram *program)
{
	u64 hash = hash::generic(program, sizeof(ShaderProgram));

	if (pipeline_layout_cache.find(hash) == pipeline_layout_cache.end())
		pipeline_layout_cache[hash] = create_pipeline_layout(program);

	return pipeline_layout_cache[hash];
}

void Device::destroy_pipeline_layout(VkPipelineLayout layout)
{
	vkDestroyPipelineLayout(device, layout, nullptr);
}

VkPipeline Device::create_pipeline(const GraphicsPipelineDef &def, VkPipelineLayout layout)
{
	assert(!def.program->is_compute());

	static const VkDynamicState graphics_pipeline_dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_BLEND_CONSTANTS // TODO: Add dynamic blend constants.
	};

	// We use vertex pulling in shaders so explicitly defined vertex formats aren't used.
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_state_create_info.pVertexBindingDescriptions = nullptr;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = nullptr; // Using dynamic viewport.
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = nullptr; // Using dynamic scissor.

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.f;
	rasterization_state_create_info.cullMode = def.cull_mode;
	rasterization_state_create_info.frontFace = def.front_face;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.f;
	rasterization_state_create_info.depthBiasClamp = 0.f;
	rasterization_state_create_info.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = def.min_sample_shading_enabled;
	multisample_state_create_info.minSampleShading = def.min_sample_shading;
	multisample_state_create_info.rasterizationSamples = def.samples;
	multisample_state_create_info.pSampleMask = nullptr;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	ScratchArena scratch;

	VkPipelineColorBlendAttachmentState *blend_states = scratch.get_arena().push_array<VkPipelineColorBlendAttachmentState>(def.colour_attachment_formats.size());

	VkPipelineColorBlendAttachmentState *blend_state = blend_states;

	for (int i = 0; i < def.colour_attachment_formats.size(); i++, blend_state++) {

		blend_state->blendEnable = def.blend_state.enabled;

		blend_state->srcColorBlendFactor = def.blend_state.colour.src;
		blend_state->dstColorBlendFactor = def.blend_state.colour.dst;
		blend_state->colorBlendOp = def.blend_state.colour.op;

		blend_state->srcAlphaBlendFactor = def.blend_state.alpha.src;
		blend_state->dstAlphaBlendFactor = def.blend_state.alpha.dst;
		blend_state->alphaBlendOp = def.blend_state.alpha.op;

		if (def.blend_state.write_mask[0]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (def.blend_state.write_mask[1]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (def.blend_state.write_mask[2]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (def.blend_state.write_mask[3]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {};
	colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_create_info.logicOpEnable = def.blend_state.logic_op_enabled;
	colour_blend_state_create_info.logicOp = def.blend_state.logic_op;
	colour_blend_state_create_info.attachmentCount = def.colour_attachment_formats.size();
	colour_blend_state_create_info.pAttachments = blend_states;
	colour_blend_state_create_info.blendConstants[0] = def.blend_state.constants[0];
	colour_blend_state_create_info.blendConstants[1] = def.blend_state.constants[1];
	colour_blend_state_create_info.blendConstants[2] = def.blend_state.constants[2];
	colour_blend_state_create_info.blendConstants[3] = def.blend_state.constants[3];

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
	depth_stencil_state_create_info.depthTestEnable       = def.depth_stencil_state.depth_test_enabled;
	depth_stencil_state_create_info.depthWriteEnable      = def.depth_stencil_state.depth_write_enabled;
	depth_stencil_state_create_info.depthCompareOp        = def.depth_stencil_state.depth_compare_op;
	depth_stencil_state_create_info.depthBoundsTestEnable = def.depth_stencil_state.depth_bounds_test_enabled;
	depth_stencil_state_create_info.minDepthBounds        = def.depth_stencil_state.depth_bounds_min;
	depth_stencil_state_create_info.maxDepthBounds        = def.depth_stencil_state.depth_bounds_max;
	depth_stencil_state_create_info.stencilTestEnable     = def.depth_stencil_state.stencil_test_enabled;
	depth_stencil_state_create_info.front.failOp          = def.depth_stencil_state.stencil_front.fail_op;
	depth_stencil_state_create_info.front.passOp          = def.depth_stencil_state.stencil_front.pass_op;
	depth_stencil_state_create_info.front.depthFailOp     = def.depth_stencil_state.stencil_front.depth_fail_op;
	depth_stencil_state_create_info.front.compareOp       = def.depth_stencil_state.stencil_front.compare_op;
	depth_stencil_state_create_info.front.writeMask       = def.depth_stencil_state.stencil_front.write_mask;
	depth_stencil_state_create_info.front.reference       = def.depth_stencil_state.stencil_front.reference;
	depth_stencil_state_create_info.back.failOp           = def.depth_stencil_state.stencil_back.fail_op;
	depth_stencil_state_create_info.back.passOp           = def.depth_stencil_state.stencil_back.pass_op;
	depth_stencil_state_create_info.back.depthFailOp      = def.depth_stencil_state.stencil_back.depth_fail_op;
	depth_stencil_state_create_info.back.compareOp        = def.depth_stencil_state.stencil_back.compare_op;
	depth_stencil_state_create_info.back.writeMask        = def.depth_stencil_state.stencil_back.write_mask;
	depth_stencil_state_create_info.back.reference        = def.depth_stencil_state.stencil_back.reference;

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = array_size(graphics_pipeline_dynamic_states);
	dynamic_state_create_info.pDynamicStates = graphics_pipeline_dynamic_states;

	VkFormat depth_stencil_format = def.has_depth_attachment
		? depth_format
		: VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.viewMask = def.view_mask;
	pipeline_rendering_create_info.colorAttachmentCount = def.colour_attachment_formats.size();
	pipeline_rendering_create_info.pColorAttachmentFormats = def.colour_attachment_formats.data();
	pipeline_rendering_create_info.depthAttachmentFormat = depth_stencil_format;
	pipeline_rendering_create_info.stencilAttachmentFormat = depth_stencil_format;

	VkPipelineShaderStageCreateInfo shader_stages[2] = {};

	for (int i = 0; i < def.program->get_stage_count(); i++) {
		VkPipelineShaderStageCreateInfo *shader_stage = &shader_stages[i];
		shader_stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage->stage = (VkShaderStageFlagBits)def.program->get_stage(i).type;
		shader_stage->module = def.program->get_stage(i).module;
		shader_stage->pName = "main";
	}

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = def.program->get_stage_count();
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

	GFX_VK_CHECK(
		vkCreateGraphicsPipelines(
			device,
			pipeline_process_cache,
			1, &graphics_pipeline_create_info,
			nullptr, &pipeline
		),
		"Failed to create graphics pipeline."
	);

	return pipeline;
}

VkPipeline Device::create_pipeline(const ComputePipelineDef &def, VkPipelineLayout layout)
{
	assert(def.program->is_compute());

	VkPipelineShaderStageCreateInfo shader_stage = {};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage = (VkShaderStageFlagBits)def.program->get_stage(0).type;
	shader_stage.module = def.program->get_stage(0).module;
	shader_stage.pName = "main";

	VkComputePipelineCreateInfo compute_pipeline_create_info = {};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.stage = shader_stage;

	VkPipeline pipeline = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreateComputePipelines(
			device,
			pipeline_process_cache,
			1, &compute_pipeline_create_info,
			nullptr, &pipeline
		),
		"Failed to create compute pipeline."
	);

	return pipeline;
}

PipelineState Device::fetch_pipeline(const GraphicsPipelineDef &def)
{
	VkPipelineLayout layout = fetch_pipeline_layout(def.program);

	u64 hash = 0;
	hash = hash::generic_combine(hash, &def.program,                    sizeof(ShaderProgram));
	hash = hash::generic_combine(hash, &def.cull_mode,                  sizeof(VkCullModeFlags));
	hash = hash::generic_combine(hash, &def.front_face,                 sizeof(VkFrontFace));
	hash = hash::generic_combine(hash, &def.blend_state,                sizeof(BlendState));
	hash = hash::generic_combine(hash, &def.depth_stencil_state,        sizeof(DepthStencilState));
	hash = hash::generic_combine(hash, &def.has_depth_attachment,       sizeof(bool));
	hash = hash::generic_combine(hash, &def.samples,                    sizeof(VkSampleCountFlagBits));
	hash = hash::generic_combine(hash, &def.min_sample_shading_enabled, sizeof(bool));
	hash = hash::generic_combine(hash, &def.min_sample_shading,         sizeof(float));
	hash = hash::generic_combine(hash, &def.view_mask,                  sizeof(u32));

	for (auto &format : def.colour_attachment_formats)
		hash = hash::generic_combine(hash, &format, sizeof(format));

	if (pipeline_cache.find(hash) == pipeline_cache.end())
		pipeline_cache[hash] = create_pipeline(def, layout);

	PipelineState st = {};
	st.pipeline = pipeline_cache[hash];
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	return st;
}

PipelineState Device::fetch_pipeline(const ComputePipelineDef &def)
{
	VkPipelineLayout layout = fetch_pipeline_layout(def.program);

	u64 hash = hash::generic(&def.program, sizeof(ShaderProgram));

	if (pipeline_cache.find(hash) == pipeline_cache.end())
		pipeline_cache[hash] = create_pipeline(def, layout);

	PipelineState st = {};
	st.pipeline = pipeline_cache[hash];
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

	return st;
}

void Device::destroy_pipeline(VkPipeline pipeline)
{
	vkDestroyPipeline(device, pipeline, nullptr);
}

Sampler *Device::create_sampler(
	VkFilter filter,
	VkSamplerAddressMode wrap_x,
	VkSamplerAddressMode wrap_y,
	VkSamplerAddressMode wrap_z,
	VkBorderColor border_colour
)
{
	VkPhysicalDeviceProperties properties =	physical_device_properties.properties;

	VkSamplerCreateInfo create_info = {};
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

	Sampler *sampler = new Sampler();
	sampler->filter = filter;
	sampler->wrap_x = wrap_x;
	sampler->wrap_y = wrap_y;
	sampler->wrap_z = wrap_z;
	sampler->border_colour = border_colour;

	GFX_VK_CHECK(
		vkCreateSampler(
			device,
			&create_info, nullptr,
			&sampler->handle
		),
		"Failed to create texture sampler."
	);

	sampler->bindless_handle = bindless.register_sampler(sampler->handle);

	return sampler;
}

void Device::destroy_sampler(const Sampler *sampler)
{
	vkDestroySampler(device, sampler->handle, nullptr);
	delete sampler;
}

static u32 clamp_mimap_count(u32 mipmaps, u32 w, u32 h, u32 d)
{
	return CalcF::min(mipmaps, 1u + (u32)CalcF::log2(CalcF::max(w, CalcF::max(h, d))));
}

Texture *Device::alloc_texture(
	u32 width, u32 height, u32 depth,
	VkFormat format, VkImageType type, VkImageTiling tiling,
	u32 mipmaps, u32 layers,
	VkSampleCountFlags samples,
	bool is_transient, bool is_storage, bool is_cubemap
)
{
	Texture *texture = new Texture();

	texture->width = width;
	texture->height = height;
	texture->depth = depth;

	texture->format = format;
	texture->type = type;
	texture->tiling = tiling;

	texture->mipmap_count = clamp_mimap_count(mipmaps, width, height, depth);
	texture->layer_count = layers;
	texture->sample_count = samples;

	if (is_transient)
		texture->usage =
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	else
		texture->usage =
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	texture->is_transient_texture = is_transient;
	texture->is_depth_texture = format == get_depth_format();
	texture->is_cubemap_texture = is_cubemap;
	texture->is_storage_texture = is_storage;
	texture->is_swapchain_texture = false;

	if (is_storage)
		texture->usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (texture->is_depth())
		texture->usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	else
		texture->usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	texture->aspect_flags = texture->is_depth()
		? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
		: VK_IMAGE_ASPECT_COLOR_BIT;

	texture->aspect_count = 0;

	for (VkImageAspectFlags b = 1; b <= texture->aspect_flags; b <<= 1) {
		if (texture->aspect_flags & b)
			texture->aspect_count++;
	}

	VkImageCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = type;
	create_info.extent.width = texture->width;
	create_info.extent.height = texture->height;
	create_info.extent.depth = texture->depth;
	create_info.mipLevels = texture->mipmap_count;
	create_info.arrayLayers = texture->layer_count;
	create_info.format = texture->format;
	create_info.tiling = texture->tiling;
	create_info.usage = texture->usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = (VkSampleCountFlagBits)texture->sample_count;
	create_info.flags = 0;

	if (is_cubemap)
		create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VmaAllocationCreateInfo vma_alloc_info = {};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	vma_alloc_info.priority = 1.f;

	GFX_VK_CHECK(
		vmaCreateImage(
			vma_allocator, &create_info,
			&vma_alloc_info, &texture->handle,
			&texture->allocation, &texture->allocation_info
		),
		"Failed to allocate texture."
	);

	return texture;
}

Texture *Device::alloc_texture_2d(u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	return alloc_texture(
		width, height, 1,
		format,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		mipmaps, 1,
		VK_SAMPLE_COUNT_1_BIT,
		false, false, false
	);
}

Texture *Device::alloc_texture_2d_rw(u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	return alloc_texture(
		width, height, 1,
		format,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		mipmaps, 1,
		VK_SAMPLE_COUNT_1_BIT,
		false, true, false
	);
}

Texture *Device::alloc_texture_2d_depth(u32 width, u32 height, u32 mipmaps)
{
	return alloc_texture_2d(
		width, height,
		depth_format,
		mipmaps
	);
}

Texture *Device::alloc_texture_2d_rw_depth(u32 width, u32 height, u32 mipmaps)
{
	return alloc_texture_2d_rw(
		width, height,
		depth_format,
		mipmaps
	);
}

Texture *Device::alloc_texture_cubemap(u32 resolution, VkFormat format, u32 mipmaps)
{
	return alloc_texture(
		resolution, resolution, 1,
		format,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		mipmaps, 6,
		VK_SAMPLE_COUNT_1_BIT,
		false, false, true
	);
}

Texture *Device::alloc_texture_cubemap_depth(u32 resolution, u32 mipmaps)
{
	return alloc_texture(
		resolution, resolution, 1,
		depth_format,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		mipmaps, 6,
		VK_SAMPLE_COUNT_1_BIT,
		false, false, true
	);
}

void Device::destroy_texture(const Texture *texture)
{
	if (!texture)
		return;

	vmaDestroyImage(vma_allocator, texture->get_handle(), texture->get_allocation());
	delete texture;
}

TextureView *Device::create_texture_view(
	const Texture *texture,
	VkImageViewType type,
	const SubresourceRange &range
)
{
	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = texture->get_handle();
	view_create_info.viewType = type;
	view_create_info.format = texture->get_format();
	
	SubresourceRange r = range.of_texture(texture);

	view_create_info.subresourceRange.aspectMask = r.aspects;
	view_create_info.subresourceRange.baseMipLevel = r.base_mip;
	view_create_info.subresourceRange.levelCount = r.mips;
	view_create_info.subresourceRange.baseArrayLayer = r.base_layer;
	view_create_info.subresourceRange.layerCount = r.layers;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	TextureView *view = new TextureView();
	view->type = type;
	view->range = r;

	GFX_VK_CHECK(
		vkCreateImageView(
			device,
			&view_create_info, nullptr,
			&view->handle
		),
		"Failed to create texture image view."
	);

	// Swapchain images are omitted from being accessible bindlessly.
	if (!texture->is_swapchain()) {
		view->bindless_handle_sampled = bindless.register_sampled(view->handle);

		if (texture->is_storage())
			view->bindless_handle_storage = bindless.register_storage(view->handle);
	}

	return view;
}

TextureView *Device::fetch_texture_view(
	const Texture *texture,
	VkImageViewType type,
	const SubresourceRange &range
)
{
	u64 hash = 0;
	hash = hash::generic_combine(hash, &texture->get_handle(),  sizeof(VkImage)); // TODO: texture.get_cookie() function
	hash = hash::generic_combine(hash, &type,                   sizeof(VkImageViewType));
	hash = hash::generic_combine(hash, &range,                  sizeof(SubresourceRange));

	if (texture_view_cache.find(hash) == texture_view_cache.end())
		texture_view_cache[hash] = create_texture_view(texture, type, range);

	return texture_view_cache[hash];
}

TextureView *Device::fetch_texture_view_std(const Texture *texture)
{
	SubresourceRange range = {};
	range.aspects = texture->is_depth() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	range.base_mip = 0;
	range.mips = texture->get_mipmap_count();
	range.base_layer = 0;
	range.layers = texture->get_layer_count();

	return fetch_texture_view(
		texture, texture->get_default_view_type(),
		range
	);
}

void Device::destroy_texture_view(const TextureView *texture_view)
{
	vkDestroyImageView(device, texture_view->get_handle(), nullptr);
	delete texture_view;
}

GpuBuffer *Device::alloc_buffer(VkBufferUsageFlags2 usage, VmaAllocationCreateFlagBits flags, u64 size)
{
	GpuBuffer *buffer = new GpuBuffer();
	buffer->usage = usage;
	buffer->size = size;
	buffer->allocator = &vma_allocator;
	buffer->allocation_flags = flags;

	if (buffer->is_storage())
		buffer->usage |= VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = buffer->size;
	buffer_create_info.usage = buffer->usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = nullptr;

	VmaAllocationCreateInfo vma_alloc_info = {};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	GFX_VK_CHECK(
		vmaCreateBuffer(
			vma_allocator,
			&buffer_create_info,
			&vma_alloc_info,
			&buffer->handle,
			&buffer->allocation,
			&buffer->allocation_info
		),
		"Failed to allocate buffer."
	);

	if (buffer->is_storage()) {
		VkBufferDeviceAddressInfo address_info = {};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer->handle;

		buffer->device_address = vkGetBufferDeviceAddress(device, &address_info);
	}

	return buffer;
}

void Device::destroy_buffer(const GpuBuffer *buffer)
{
	if (!buffer)
		return;

	vmaDestroyBuffer(vma_allocator, buffer->get_handle(), buffer->get_allocation());
	delete buffer;
}

// TODO: Move elsewhere.
struct FileBytes {
	b8 *data;
	u64 length;
};

static FileBytes load_file_bytes(MemoryArena &dst, const String &path)
{
	b8 *bytes = nullptr;

	FILE *file = fopen(path.c_str(), "rb");
	u64 file_size = 0;

	if (file) {
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		bytes = (b8 *)dst.push(file_size);
		fread(bytes, file_size, 1, file);

		fclose(file);
	}

	return { .data = bytes, .length = file_size };
}

ShaderStage Device::load_shader_stage_from_bytecode(const String &path)
{
	ScratchArena scratch;

	FileBytes source = load_file_bytes(scratch.get_arena(), path);

	SpvReflectShaderModule reflect_module = {};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(source.length, source.data, &reflect_module);

	if (reflect_result != SPV_REFLECT_RESULT_SUCCESS)
		debug_log_crash("Failed to reflect SPIR-V module: %d\n", reflect_result);

	ShaderStage stage = {};

	if (reflect_module.entry_point_count >= 1) {
		stage.type = (VkShaderStageFlags)reflect_module.entry_points[0].shader_stage;

		u32 push_constant_count = 0;
		reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, nullptr);

		if (reflect_result == SPV_REFLECT_RESULT_SUCCESS && push_constant_count > 0) {
			SpvReflectBlockVariable **pcs = scratch.get_arena().push_array<SpvReflectBlockVariable *>(push_constant_count);
			spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, pcs);

			for (u32 i = 0; i < push_constant_count; i++) {
				SpvReflectBlockVariable *pc = pcs[i];

				u32 alignment = 4;

				for (u32 j = 0; j < pc->member_count; j++)
					alignment = CalcU::max(alignment, pc->members[j].size);

				u32 padded = memory_align_up(pc->size, alignment);
				stage.push_constant_size = CalcU::max(stage.push_constant_size, padded);
			}
		}

		VkShaderModuleCreateInfo module_create_info = {};
		module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_create_info.codeSize = source.length;
		module_create_info.pCode = (const u32 *)source.data;

		GFX_VK_CHECK(
			vkCreateShaderModule(device,
				&module_create_info, nullptr,
				&stage.module
			),
			"Failed to create shader module."
		);
	} else {
		debug_log_crash("No entry points found in SPIR-V.\n");
	}

	spvReflectDestroyShaderModule(&reflect_module);

	return stage;
}

void Device::destroy_shader_stage(const ShaderStage &stage)
{
	vkDestroyShaderModule(device, stage.module, nullptr);
}

ShaderProgram *Device::create_shader_program(const Vector<String> &stage_paths)
{
	ShaderProgram *program = new ShaderProgram();
	program->stage_count = stage_paths.size();

	for (int i = 0; i < program->stage_count; i++) {
		const String &path = stage_paths[i];
		program->stages[i] = load_shader_stage_from_bytecode(path);
		program->push_constant_size = CalcU::max(program->push_constant_size, program->stages[i].push_constant_size);
	}

	return program;
}

void Device::destroy_shader_program(const ShaderProgram *program)
{
	for (int i = 0; i < program->stage_count; i++)
		destroy_shader_stage(program->stages[i]);

	delete program;
}

static VkDescriptorType get_descriptor_type_from_bindless_set(BindlessSetKind kind)
{
	switch (kind) {
		case BINDLESS_SET_SAMPLER:  return VK_DESCRIPTOR_TYPE_SAMPLER;
		case BINDLESS_SET_SAMPLED:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case BINDLESS_SET_STORAGE:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}

	debug_log_crash("Could not find descriptor type from bindless binding type.");

	return (VkDescriptorType)0;
}

void Device::create_bindless()
{
	VkDescriptorPoolSize pool_sizes[BINDLESS_SET_MAX_ENUM] = {};

	for (int i = 0; i < BINDLESS_SET_MAX_ENUM; i++) {
		pool_sizes[i].type = get_descriptor_type_from_bindless_set((BindlessSetKind)i);
		pool_sizes[i].descriptorCount = BindlessResources::MAX_RESOURCES;
	}

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = array_size(pool_sizes) * BindlessResources::MAX_RESOURCES;
	pool_create_info.poolSizeCount = array_size(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;

	GFX_VK_CHECK(
		vkCreateDescriptorPool(device,
			&pool_create_info, nullptr,
			&bindless.pool
		),
		"Failed to create bindless descriptor pool."
	);

	VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = 1;
	binding_flags.pBindingFlags = &bindless_flags;

	for (int i = 0; i < BINDLESS_SET_MAX_ENUM; i++) {
		VkDescriptorSetLayoutBinding binding = {};
		binding.descriptorType = get_descriptor_type_from_bindless_set((BindlessSetKind)i);
		binding.descriptorCount = BindlessResources::MAX_RESOURCES;
		binding.binding = 0;
		binding.stageFlags = VK_SHADER_STAGE_ALL;
		binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layout_create_info = {};
		layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_create_info.bindingCount = 1;
		layout_create_info.pBindings = &binding;
		layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		layout_create_info.pNext = &binding_flags;

		GFX_VK_CHECK(
			vkCreateDescriptorSetLayout(device,
				&layout_create_info, nullptr,
				&bindless.layouts[i]
			),
			"Failed to create bindless descriptor layout."
		);
	}

	// ---

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = bindless.pool;
	alloc_info.descriptorSetCount = BINDLESS_SET_MAX_ENUM;
	alloc_info.pSetLayouts = bindless.layouts;

	GFX_VK_CHECK(
		vkAllocateDescriptorSets(device, &alloc_info, bindless.sets),
		"Failed to allocate bindless descriptor set."
	);

	debug_log("Bindless resources created.");
}

void Device::destroy_bindless()
{
	for (int i = 0; i < BINDLESS_SET_MAX_ENUM; i++)
		vkDestroyDescriptorSetLayout(device, bindless.layouts[i], nullptr);

	vkDestroyDescriptorPool(device, bindless.pool, nullptr);
}

void Device::apply_bindless_updates()
{
	if (bindless.updates.empty())
		return;

	Vector<VkWriteDescriptorSet> descriptor_writes(bindless.updates.size());
	Vector<VkDescriptorImageInfo> image_infos(bindless.updates.size());

	for (int i = 0; i < bindless.updates.size(); i++) {
		BindlessResources::BindlessUpdate &update = bindless.updates[i];

		VkDescriptorImageInfo *image_info = &image_infos[i];
		image_info->sampler = update.sampler;
		image_info->imageView = update.view;
		image_info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet *write = &descriptor_writes[i];
		write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write->descriptorCount = 1;
		write->dstArrayElement = update.slot;
		write->descriptorType = get_descriptor_type_from_bindless_set(update.kind);
		write->dstSet = bindless.sets[update.kind];
		write->dstBinding = 0;
		write->pImageInfo = image_info;
	}

	vkUpdateDescriptorSets(
		device,
		descriptor_writes.size(), descriptor_writes.data(),
		0, nullptr
	);

	bindless.updates.clear();
}

void Device::init_imgui()
{
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

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = max_sets;
	pool_info.poolSizeCount = array_size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	GFX_VK_CHECK(
		vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_pool),
		"Failed to create ImGui descriptor pool."
	);

	VkFormat swapchain_image_format = VK_FORMAT_R32G32B32A32_SFLOAT;

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physical_device;
	init_info.Device = device;
	init_info.QueueFamily = graphics_queue.get_family_index();
	init_info.Queue = graphics_queue.get_handle();
	init_info.PipelineCache = pipeline_process_cache;
	init_info.DescriptorPool = imgui_pool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = FRAMES_IN_FLIGHT;
	init_info.ImageCount = FRAMES_IN_FLIGHT;
	init_info.CheckVkResultFn = nullptr;
	init_info.UseDynamicRendering = true;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_image_format;

	ImGui_ImplVulkan_Init(&init_info);
}

void Device::destroy_imgui()
{
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(device, imgui_pool, nullptr);
}

void Device::imgui_new_frame()
{
	ImGui_ImplVulkan_NewFrame();
}

void Device::imgui_record_draw_data(const CommandBuffer &cmd)
{
	ImDrawData *draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, cmd.get_handle());
}
