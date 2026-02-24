#include "context.h"
#include "vk_check.h"

#include "core/scratch.h"
#include "platform/platform.h"

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

static const char *const *get_instance_extensions(ArenaView &arena, u32 *extension_count)
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
	
	vkEnumerateDeviceExtensionProperties(
		physical_device, nullptr,
		&extension_count, nullptr
	);

	if (extension_count <= 0)
		debug_log_crash("Failed to find any device extension properties.");

	ScratchScope scratch = scratch::get();

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

static bool check_for_validation_layer_support(ArenaView &arena)
{
	u32 layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, 0);

	ScratchScope scratch = scratch::get();

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

Context::Context()
	: instance()
	, device()
	, physical_device()
	, physical_device_properties()
	, physical_device_features()
	, surface()
	, vma_allocator()
	, graphics_queue()
	, debug_messenger()
	, has_validation_layers()
	, depth_format()
	, max_msaa_samples()
	, swapchain_details()
{
}

Context::~Context()
{
}

void Context::init()
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
	
	ScratchScope scratch = scratch::get();

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
	vulkan12_features.drawIndirectCount = VK_TRUE;
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
		vkCreateDevice(
			physical_device,
			&device_create_info, nullptr,
			&device
		),
		"Failed to create logical device."
	);

	debug_log("Created logical device.");
	
	vkGetDeviceQueue(
		device,
		graphics_queue.family_index, 0,
		&graphics_queue.handle
	);

	debug_log("Created graphics queue.");

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

	GFX_VK_CHECK(
		vmaCreateAllocator(
			&allocator_create_info,
			&vma_allocator
		),
		"Failed to create Vulkan Memory Allocator."
	);

	debug_log("Created Vulkan Memory Allocator.");

	swapchain_details = query_swapchain_support(physical_device, surface);
}

void Context::destroy()
{
	platform::destroy_vulkan_surface(instance, surface);
	vmaDestroyAllocator(vma_allocator);
	vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
	vkDestroyDevice(device, nullptr);
}

VkInstance Context::get_instance() const
{
	return instance;
}

VkDevice Context::get_device() const
{
	return device;
}

VkPhysicalDevice Context::get_physical_device() const
{
	return physical_device;
}

const VkPhysicalDeviceProperties2 &Context::get_physical_properties() const
{
	return physical_device_properties;
}

VkSurfaceKHR Context::get_surface() const
{
	return surface;
}

VmaAllocator Context::get_allocator() const
{
	return vma_allocator;
}

HardwareQueue Context::graphics() const
{
	return graphics_queue;
}

VkFormat Context::get_depth_format() const
{
	return depth_format;
}

VkSampleCountFlagBits Context::get_max_msaa_samples() const
{
	return max_msaa_samples;
}

SwapchainSupportDetails Context::get_swapchain_details() const
{
	return swapchain_details;
}
