#include "graphics/device.h"
#include "graphics/cmd_pool.h"
#include "graphics/util.h"
#include "graphics/vk_check.h"
#include "os/platform.h"
#include "core/hash.h"

#include <SPIRV-Reflect/spirv_reflect.h>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#define MAGPIE_VULKAN_API_VERSION VK_API_VERSION_1_4

static gfx::Device *selected_device = nullptr;

static const char *VALIDATION_LAYERS[] = {
	"VK_LAYER_KHRONOS_validation"
};

static VkFormat FindSupportedFormat(
	VkPhysicalDevice physical_device,
	VkImageTiling tiling,
	VkFormatFeatureFlags features,
	unsigned candidate_count, const VkFormat *candidates,
	LogChannel log_channel
)
{
	for (unsigned i = 0; i < candidate_count; i++) {
		VkFormatProperties properties = {};

		vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) ||
			(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)) {
			return candidates[i];
		}
	}

	DebugLogB(log_channel, "Failed to find supported format.");

	return VK_FORMAT_MAX_ENUM;
}

static VkFormat FindDepthStencilFormat(VkPhysicalDevice physical_device, LogChannel log_channel)
{
	static constexpr VkFormat candidates[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};

	return FindSupportedFormat(
		physical_device,
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
		ArraySize(candidates), candidates,
		log_channel
	);
}

static VkSampleCountFlagBits FindMaxUsableSampleCount(const VkPhysicalDeviceProperties2 &properties, LogChannel log_channel)
{
	VkSampleCountFlags counts =
		properties.properties.limits.framebufferColorSampleCounts &
		properties.properties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT)
		return VK_SAMPLE_COUNT_64_BIT;
	else if (counts & VK_SAMPLE_COUNT_32_BIT)
		return VK_SAMPLE_COUNT_32_BIT;
	else if (counts & VK_SAMPLE_COUNT_16_BIT)
		return VK_SAMPLE_COUNT_16_BIT;
	else if (counts & VK_SAMPLE_COUNT_8_BIT)
		return VK_SAMPLE_COUNT_8_BIT;
	else if (counts & VK_SAMPLE_COUNT_4_BIT)
		return VK_SAMPLE_COUNT_4_BIT;
	else if (counts & VK_SAMPLE_COUNT_2_BIT)
		return VK_SAMPLE_COUNT_2_BIT;
	else if (counts & VK_SAMPLE_COUNT_1_BIT)
		return VK_SAMPLE_COUNT_1_BIT;

	DebugLogB(log_channel, "Could not find a maximum usable sample count.");

	return VK_SAMPLE_COUNT_1_BIT;
}

static Vector<const char *> GetInstanceExtensions(LogChannel log_channel)
{
	unsigned extension_count = 0;
	const char *const *names = os::GetVulkanInstanceExtensions(&extension_count);

	if (!names)
		DebugLogB(log_channel, "Unable to get instance extension count.");

	Vector<const char *> extensions(names, names + extension_count);

	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#ifdef __APPLE__
	// VK_EXT_METAL_SURFACE_EXTENSION_NAME, using kosmickrisp
	extensions.push_back("VK_EXT_metal_surface");
	//extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

	return extensions;
}

static bool CheckForValidationLayerSupport()
{
	unsigned layer_count = 0;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	Vector<VkLayerProperties> available_layers(layer_count);

	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (unsigned i = 0; i < ArraySize(VALIDATION_LAYERS); i++) {
		bool has_layer = false;
		const char *layer_name_0 = VALIDATION_LAYERS[i];

		for (unsigned j = 0; j < layer_count; j++) {
			const char *layer_name_1 = available_layers[j].layerName;

			if (strcmp(layer_name_0, layer_name_1) == 0) {
				has_layer = true;
				break;
			}
		}

		if (!has_layer)
			return false;
	}

	return true;
}

static gfx::SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
	gfx::SwapchainSupportDetails result;

	unsigned surface_format_count = 0;
	unsigned present_mode_count = 0;

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

static VkResult CreateDebugUtilsMessengerExt(
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

static bool FormatSupportsBlitDst(VkPhysicalDevice physical_device, VkFormat format)
{
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
	return (props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0;
}

static VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(
	LogChannel channel,
	VkPhysicalDevice physical_device,
	const Vector<VkSurfaceFormatKHR> &available_surface_formats
)
{
	static const VkFormat preferred[] = {
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_R8G8B8A8_SRGB,
	};

	for (VkFormat want : preferred) {
		for (const VkSurfaceFormatKHR &format : available_surface_formats) {
			if (format.format == want && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && FormatSupportsBlitDst(physical_device, want)) {
				DebugLogD(channel, "Chose swapchain format: %d.", want);
				return format;
			}
		}
	}

	DebugLogB(channel, "Could not find desired swapchain surface format and colour space.");

	return available_surface_formats[0];
}

static VkPresentModeKHR ChooseSwapchainPresentMode(
	const Vector<VkPresentModeKHR> &available_present_modes,
	bool enable_vsync
)
{
	if (!enable_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for (VkPresentModeKHR mode : available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseSwapchainExtent(const os::Window *window, const VkSurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != ((unsigned)-1) &&
		capabilities.currentExtent.height != ((unsigned)-1)) {
		return capabilities.currentExtent;
	}

	unsigned window_width = 0;
	unsigned window_height = 0;

	window->GetSize(&window_width, &window_height);

	VkExtent2D actual_extent = {
		window_width,
		window_height
	};

	actual_extent.width = maths::Clamp(
		actual_extent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width
	);

	actual_extent.height = maths::Clamp(
		actual_extent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height
	);

	return actual_extent;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *ctx
)
{
	gfx::Device *device = (gfx::Device *)ctx;

	LogChannel ch = device->log_channel;

	switch (message_type) {
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

	switch (message_severity) {
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

		default:
			break;
	}

	return VK_FALSE;
}

gfx::Device *gfx::Device::Get()
{
	return selected_device;
}

void gfx::Device::Select(Device *device)
{
	selected_device = device;
}

gfx::Device::Device(os::Window *window_, LogChannel log_channel_, const Vector<FeatureRequest> &requested_features)
	: window(window_)
	, log_channel(log_channel_)
	, vk_instance(VK_NULL_HANDLE)
	, vk_device(VK_NULL_HANDLE)
	, vk_physical_device(VK_NULL_HANDLE)
	, vk_physical_device_properties()
	, vk_physical_device_features()
	, vk_surface(VK_NULL_HANDLE)
	, vma_allocator(VK_NULL_HANDLE)
	, pipeline_process_cache(VK_NULL_HANDLE)
	, has_validation_layers(false)
	, debug_messenger(VK_NULL_HANDLE)
	, default_depth_stencil_format(VK_FORMAT_UNDEFINED)
	, max_msaa_samples(VK_SAMPLE_COUNT_1_BIT)
	, max_push_constants_size(0)
	, current_frame_in_flight_index(0)
	, retire_frame(0)
	, immediate_pool(nullptr)
	, imgui_enabled(false)
	, imgui_pool(VK_NULL_HANDLE)
{
	log_channel_general = logs::OpenChannel(log_channel, "GENERAL");
	log_channel_validation = logs::OpenChannel(log_channel, "VALIDATION");
	log_channel_performance = logs::OpenChannel(log_channel, "PERFORMANCE");
	log_channel_cmd_buffer = logs::OpenChannel(log_channel, "CMD");

	Select(this);

	Requirements requirements;

	requirements.base_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
		VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
		VK_KHR_MAINTENANCE_6_EXTENSION_NAME,
	};

	for (const FeatureRequest &request : requested_features) {
		Feature feature;
		feature.def = GetFeatureDef(request.type);
		feature.type = request.type;
		feature.tier = request.tier;

		requirements.features.push_back(feature);
	}

	VkApplicationInfo core_info = {};

	core_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	core_info.pApplicationName = OS_APPLICATION_NAME;
	core_info.pEngineName = OS_ENGINE_NAME;

	core_info.applicationVersion = VK_MAKE_API_VERSION(
		0,
		OS_APP_VERSION_MAJOR,
		OS_APP_VERSION_MINOR,
		OS_APP_VERSION_PATCH
	);

	core_info.engineVersion = VK_MAKE_API_VERSION(
		0,
		OS_ENGINE_VERSION_MAJOR,
		OS_ENGINE_VERSION_MINOR,
		OS_ENGINE_VERSION_PATCH
	);

	core_info.apiVersion = MAGPIE_VULKAN_API_VERSION;

	VkInstanceCreateInfo instance_create_info = {};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &core_info;

	volkInitialize();

	Vector<const char *> instance_extensions = GetInstanceExtensions(log_channel);

	instance_create_info.enabledExtensionCount = (unsigned)instance_extensions.size();
	instance_create_info.ppEnabledExtensionNames = instance_extensions.data();

	static const VkValidationFeatureEnableEXT enabled_features[] = {
		//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT
	};

	VkValidationFeaturesEXT validation_features = {};
	validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validation_features.enabledValidationFeatureCount = ArraySize(enabled_features);
	validation_features.pEnabledValidationFeatures = enabled_features;

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
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

	debug_create_info.pfnUserCallback = VulkanDebugCallback;
	debug_create_info.pUserData = this;

	has_validation_layers = CheckForValidationLayerSupport();

	if (has_validation_layers) {
		DebugLogD(log_channel, "Validation layer support verified.");

		instance_create_info.enabledLayerCount = ArraySize(VALIDATION_LAYERS);
		instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS;
		instance_create_info.pNext = &debug_create_info;
	} else {
		DebugLogW(log_channel, "No validation layer support.");

		instance_create_info.enabledLayerCount = 0;
		instance_create_info.ppEnabledLayerNames = nullptr;
		instance_create_info.pNext = nullptr;
	}

	// kosmickrisp
#ifdef __APPLE__
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	GRAPHICS_VK_CHECK(
		vkCreateInstance(&instance_create_info, nullptr, &vk_instance),
		"Failed to create instance."
	);

	volkLoadInstance(vk_instance);

	if (has_validation_layers) {
		GRAPHICS_VK_CHECK(
			CreateDebugUtilsMessengerExt(vk_instance,
				&debug_create_info, nullptr,
				&debug_messenger
			),
			"Failed to create debug messenger."
		);
	}

	if (!window->CreateVulkanSurface(vk_instance, &vk_surface))
		DebugLogB(log_channel, "Failed to create surface.");

	// Enumerate physical devices.
	{
		unsigned physical_device_count = 0;
		vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, nullptr);

		DebugLogAssert(log_channel, physical_device_count > 0, "Failed to find GPUs with Vulkan support.");

		DebugLogD(log_channel, "Found %u physical devices.", physical_device_count);

		Vector<VkPhysicalDevice> physical_devices(physical_device_count);

		vkEnumeratePhysicalDevices(vk_instance, &physical_device_count, physical_devices.data());

		unsigned best_usability = 0;

		for (unsigned i = 0; i < physical_device_count; i++) {
			VkPhysicalDevice curr_physical_device = physical_devices[i];

			VkPhysicalDeviceProperties2 properties = {};
			properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

			vkGetPhysicalDeviceProperties2(curr_physical_device, &properties);

			DebugLogD(
				log_channel,
				"Querying physical device: \"%s\" (%d)",
				properties.properties.deviceName,
				properties.properties.deviceID
			);

			bool has_essentials = true;

			Capabilities detected_caps;
			ResolvedFeatures resolved_features;
			VkPhysicalDeviceFeatures2 device_features = {};

			// swapchain
			{
				SwapchainSupportDetails details = QuerySwapchainSupport(curr_physical_device, vk_surface);

				if (details.surface_formats.empty() ||
					details.present_modes.empty()) {
					has_essentials = false;
				}
			}

			// capabilities
			{
				detected_caps = Capabilities::Query(curr_physical_device, &device_features, log_channel);
				resolved_features = ResolvedFeatures::Resolve(curr_physical_device, detected_caps, requirements, log_channel);

				if (!resolved_features.meets_requirements)
					has_essentials = false;
			}

			unsigned current_usability = requirements.Score(resolved_features.enabled);

			if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				current_usability += 4;
			else if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				current_usability += 1;

			bool better =
				current_usability > best_usability || // is it better than our current device...?
				vk_physical_device == VK_NULL_HANDLE; // ...or is it the first one we're seeing?

			if (better && has_essentials) {
				vk_physical_device = curr_physical_device;
				vk_physical_device_properties = properties;
				vk_physical_device_features = device_features;

				features = resolved_features;

				best_usability = current_usability;
			}
		}

		DebugLogAssert(
			log_channel,
			vk_physical_device,
			"Unable to find a suitable physical device :("
		);

		DebugLogD(
			log_channel,
			"Selected physical device: \"%s\" (%d)...     :3",
			vk_physical_device_properties.properties.deviceName,
			vk_physical_device_properties.properties.deviceID
		);
	}

	max_msaa_samples = FindMaxUsableSampleCount(vk_physical_device_properties, log_channel);
	max_push_constants_size = vk_physical_device_properties.properties.limits.maxPushConstantsSize;

	default_depth_stencil_format = FindDepthStencilFormat(vk_physical_device, log_channel);

	unsigned queue_family_count = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr);

	if (queue_family_count <= 0)
		DebugLogD(log_channel, "Failed to find any queue families.");
	else
		DebugLogD(log_channel, "Found %u queue families.", queue_family_count);

	Vector<VkQueueFamilyProperties> queue_families(queue_family_count);

	vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, queue_families.data());

	for (int i = 0; i < queue_family_count; i++) {
		if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
			VkBool32 present_support = VK_FALSE;

			vkGetPhysicalDeviceSurfaceSupportKHR(
				vk_physical_device, i,
				vk_surface,
				&present_support
			);

			if (present_support) {
				DebugLogD(log_channel, "Selected graphics queue with family index: %u", i);

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

	VkPhysicalDeviceFeatures enabled_base_features = {};
	enabled_base_features.samplerAnisotropy = VK_TRUE;
	enabled_base_features.sampleRateShading = VK_TRUE;
	enabled_base_features.shaderInt64 = VK_TRUE;
	enabled_base_features.multiDrawIndirect = VK_TRUE;
	//enabled_base_features.robustBufferAccess = VK_TRUE;
	enabled_base_features.textureCompressionBC = SupportsTextureCompressionBC(); // typically only on desktops
	enabled_base_features.textureCompressionASTC_LDR = SupportsTextureCompressionASTC(); // typically only on mobiles

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
	vulkan12_features.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
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

	/*
	  VkPhysicalDeviceVulkan14Features vulkan14_features = {};
	  vulkan14_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
	  vulkan14_features.maintenance5 = VK_TRUE;
	  vulkan14_features.pNext = &vulkan13_features;
	*/

	VkPhysicalDeviceMaintenance5FeaturesKHR mt5_features = {};
	mt5_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR;
	mt5_features.maintenance5 = VK_TRUE;
	mt5_features.pNext = &vulkan13_features;

	VkPhysicalDeviceMaintenance6FeaturesKHR mt6_features = {};
	mt6_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES_KHR;
	mt6_features.maintenance6 = VK_TRUE;
	mt6_features.pNext = &mt5_features;

	void *feature_chain_tail = &mt6_features;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_features = {};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_struct_features = {};
	VkPhysicalDeviceRayQueryFeaturesKHR ray_query_features = {};

	if (FeatureEnabled(FeatureType_RayTracing)) {
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

	VkDeviceCreateInfo device_create_info = {};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &graphics_queue_create_info;
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = nullptr;
	device_create_info.enabledExtensionCount = features.extension_names.size();
	device_create_info.ppEnabledExtensionNames = features.extension_names.data();
	device_create_info.pEnabledFeatures = &enabled_base_features;
	device_create_info.pNext = feature_chain_tail;

	GRAPHICS_VK_CHECK(
		vkCreateDevice(vk_physical_device,
			&device_create_info, nullptr,
			&vk_device),
		"Failed to create logical device."
	);

	DebugLogD(log_channel, "Created logical device.");

	vkGetDeviceQueue(
		vk_device,
		graphics_queue.family_index, 0,
		&graphics_queue.vk_handle
	);

	unsigned instance_version = 0;
	VkResult version_result = vkEnumerateInstanceVersion(&instance_version);

	if (version_result == VK_SUCCESS) {
		unsigned device_version = vk_physical_device_properties.properties.apiVersion;

		unsigned i_major = VK_API_VERSION_MAJOR(instance_version);
		unsigned i_minor = VK_API_VERSION_MINOR(instance_version);
		unsigned i_patch = VK_API_VERSION_PATCH(instance_version);

		unsigned d_major = VK_API_VERSION_MAJOR(device_version);
		unsigned d_minor = VK_API_VERSION_MINOR(device_version);
		unsigned d_patch = VK_API_VERSION_PATCH(device_version);

		DebugLogD(
			log_channel,
			"Instance Version: %u.%u.%u, Device Version: %u.%u.%u",
			i_major, i_minor, i_patch,
			d_major, d_minor, d_patch
		);
	} else {
		DebugLogE(log_channel, "Failed to retrieve Vulkan version, something's gone wrong probably.");
	}

	volkLoadDevice(vk_device);

	VmaVulkanFunctions vulkan_functions = {};
	vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
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
	vulkan_functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
	vulkan_functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
	vulkan_functions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
	vulkan_functions.vkBindImageMemory2KHR = vkBindImageMemory2;
	vulkan_functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
	vulkan_functions.vkGetPhysicalDeviceProperties2KHR = vkGetPhysicalDeviceProperties2;
	vulkan_functions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
	vulkan_functions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

	VmaAllocatorCreateInfo allocator_create_info = {};
	allocator_create_info.physicalDevice = vk_physical_device;
	allocator_create_info.device = vk_device;
	allocator_create_info.instance = vk_instance;
	allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT; // buffer usage goes in through VkBufferUsageFlags2CreateInfo
	allocator_create_info.pVulkanFunctions = &vulkan_functions;
	allocator_create_info.vulkanApiVersion = MAGPIE_VULKAN_API_VERSION;

	GRAPHICS_VK_CHECK(
		vmaCreateAllocator(
			&allocator_create_info,
			&vma_allocator
		),
		"Failed to create Vulkan Memory Allocator."
	);

	DebugLogD(log_channel, "Created Vulkan Memory Allocator.");

	swapchain_details = QuerySwapchainSupport(vk_physical_device, vk_surface);

	VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = nullptr;
	pipeline_cache_create_info.flags = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = nullptr;

	GRAPHICS_VK_CHECK(
		vkCreatePipelineCache(vk_device,
			&pipeline_cache_create_info, nullptr,
			&pipeline_process_cache),
		"Failed to process pipeline cache."
	);

	CreateSyncResources();
	CreateBindless();
	CreateImGui();

	DebugLogI(log_channel, "Initialized.");
}

gfx::Device::~Device()
{
	WaitIdle();
	ReleaseRetired(0, true);

	// pipeline layouts, pipelines and texture views are special and cached
	// internally without any API to destroy them so we
	// have to destroy them here.

	pipeline_layouts.ForEachNotLocked([this](VkPipelineLayout layout) {
		vkDestroyPipelineLayout(vk_device, layout, nullptr);
	});

	pipelines.ForEachNotLocked([this](VkPipeline pipeline) {
		vkDestroyPipeline(vk_device, pipeline, nullptr);
	});

	texture_views.ForEachNotLocked([this](const TextureView &view) {
		vkDestroyImageView(vk_device, view.vk_handle, nullptr);
	});

	DestroyImGui();
	DestroyBindless();
	DestroySyncResources();

	vkDestroyPipelineCache(vk_device, pipeline_process_cache, nullptr);
	window->DestroyVulkanSurface(vk_instance, vk_surface);
	vmaDestroyAllocator(vma_allocator);

	if (debug_messenger)
		vkDestroyDebugUtilsMessengerEXT(vk_instance, debug_messenger, nullptr);

	vkDestroyDevice(vk_device, nullptr);
	vkDestroyInstance(vk_instance, nullptr);

	DebugLogI(log_channel, "Destroyed.");

	if (selected_device == this)
		selected_device = nullptr;
}

bool gfx::Device::FeatureEnabled(FeatureType type) const
{
	return features.enabled.set[type];
}

bool gfx::Device::SupportsTextureCompressionBC() const
{
	return vk_physical_device_features.features.textureCompressionBC == VK_TRUE;
}

bool gfx::Device::SupportsTextureCompressionASTC() const
{
	return vk_physical_device_features.features.textureCompressionASTC_LDR == VK_TRUE;
}

bool gfx::Device::FormatSupports(VkFormat format, VkFormatFeatureFlags required) const
{
	VkFormatProperties properties = {};
	vkGetPhysicalDeviceFormatProperties(vk_physical_device, format, &properties);
		return (properties.optimalTilingFeatures & required) == required;
}

void gfx::Device::SetRetiredFrame(u64 frame_number)
{
	retire_frame.store((i64)frame_number, std::memory_order_release);
}

// THE CALLER HOLDS THE LOCK!!!!!!!!!!!!!!!!!!!!!!!!
gfx::PendingDestroy &gfx::Device::PushPendingDestroyLocked(PendingDestroyKind kind, ResourceKey key)
{
	PendingDestroy node;
	node.kind = kind;
	node.key = key;
	node.retire_frame = (u64)retire_frame.load(std::memory_order_acquire);

	destroy_queue.push_back(node);

	return destroy_queue.back();
}

void gfx::Device::DestroyPendingNow(const PendingDestroy &d)
{
	switch (d.kind) {
		case PendingDestroyKind_Image: {
			textures.Return(d.key);
			vmaDestroyImage(vma_allocator, d.image, d.allocation);
		} break;

		case PendingDestroyKind_Buffer: {
			buffers.Return(d.key);
			vmaDestroyBuffer(vma_allocator, d.buffer, d.allocation);
		} break;

		case PendingDestroyKind_Sampler: {
			samplers.Return(d.key);
			vkDestroySampler(vk_device, d.sampler, nullptr);
			bindless.FreeSampler(d.sampler_bindless);
		} break;

		case PendingDestroyKind_Shader: {
			shaders.Return(d.key);
		} break;

		case PendingDestroyKind_AccelStruct: {
			accel_structures.Return(d.key);
			vkDestroyAccelerationStructureKHR(vk_device, d.accel, nullptr);
		} break;

		case PendingDestroyKind_ImGuiTexture: {
			// the descriptor pool takes them all with it once ImGui is gone
			if (imgui_enabled)
				ImGui_ImplVulkan_RemoveTexture(d.imgui_set);
		} break;

		default:
			break;
	}
}

// releases every queued destroy whose retire frame is <= completed_frame
// (or everything, at shutdown, after the device is idle).
void gfx::Device::ReleaseRetired(u64 completed_frame, bool release_all)
{
	for (;;) {
		destroy_queue_lock.Acquire();

		if (destroy_queue.empty() || (!release_all && destroy_queue.front().retire_frame > completed_frame)) {
			destroy_queue_lock.Release();
			break;
		}

		PendingDestroy copy = destroy_queue.front();
		destroy_queue.pop_front();

		destroy_queue_lock.Release();

		DestroyPendingNow(copy);
	}
}

// TODO: BeginFrame / EndFrame should be moved into the swapchain.

void gfx::Device::FrameWait(u64 frame_number)
{
	current_frame_in_flight_index = (unsigned)(frame_number % GRAPHICS_FRAMES_IN_FLIGHT);

	FrameInFlight *frame_in_flight = &frames_in_flight[current_frame_in_flight_index];

	WaitUntil(frame_in_flight->completion_point);

	frame_in_flight->frame_number = frame_number;

	if (frame_number >= GRAPHICS_FRAMES_IN_FLIGHT)
		ReleaseRetired(frame_number - GRAPHICS_FRAMES_IN_FLIGHT, false);

	frame_in_flight->command_pool->Purge(frame_in_flight->completion_point.frame);
}

unsigned gfx::Device::FrameSlotIndex() const
{
	return current_frame_in_flight_index;
}

gfx::CmdBuffer gfx::Device::BeginFrame(Swapchain *swapchain)
{
	FrameInFlight *frame_in_flight = &frames_in_flight[current_frame_in_flight_index];

	VkAcquireNextImageInfoKHR acquire_next_image_info = {};
	acquire_next_image_info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquire_next_image_info.swapchain = swapchain->vk_handle;
	acquire_next_image_info.timeout = UINT64_MAX;
	acquire_next_image_info.semaphore = frame_in_flight->image_available_semaphore;
	acquire_next_image_info.fence = VK_NULL_HANDLE;
	acquire_next_image_info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(vk_device, &acquire_next_image_info, &swapchain->current_frame_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		DebugLogB(log_channel, "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		DebugLogB(log_channel, "Failed to acquire next image in swapchain.");

	CmdBuffer cmd = frame_in_flight->command_pool->Acquire();
	cmd.Begin();

	return cmd;
}

void gfx::Device::EndFrame(const Swapchain *swapchain, const CmdBuffer &cmd)
{
	ApplyBindlessUpdates();

	FrameInFlight *frame_in_flight = &frames_in_flight[current_frame_in_flight_index];
	const SwapchainFrame *swapchain_frame = &swapchain->frames[swapchain->current_frame_index];

	VkSemaphoreSubmitInfo image_available_semaphore_info = {};
	image_available_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore_info.semaphore = frame_in_flight->image_available_semaphore;
	image_available_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSemaphoreSubmitInfo render_finished_semaphore_info = {};
	render_finished_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore_info.semaphore = swapchain_frame->render_finished_semaphore;
	render_finished_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // TODO: Do we need to be waiting on VK_PIPELINE_STAGE_2_ALL_COMMANDS ????

	frame_in_flight->completion_point = SubmitEx(
		cmd,
		1, &image_available_semaphore_info,
		1, &render_finished_semaphore_info
	);

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = nullptr;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain->vk_handle;
	present_info.pImageIndices = &swapchain->current_frame_index;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &swapchain_frame->render_finished_semaphore;

	queue_lock.Acquire();
	VkResult result = vkQueuePresentKHR(graphics_queue.vk_handle, &present_info);
	queue_lock.Release();

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		DebugLogB(log_channel, "TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		DebugLogB(log_channel, "Failed to present swapchain image. (%u)", result);

	frame_in_flight->command_pool->Release(cmd, frame_in_flight->completion_point.frame);
}

gfx::TimelinePoint gfx::Device::Submit(const CmdBuffer &cmd)
{
	return SubmitEx(cmd, 0, nullptr, 0, nullptr);
}

gfx::TimelinePoint gfx::Device::SubmitEx(
	const CmdBuffer &cmd,
	unsigned wait_count, const VkSemaphoreSubmitInfo *waits,
	unsigned signal_count, const VkSemaphoreSubmitInfo *signals
)
{
	cmd.End();

	Vector<VkSemaphoreSubmitInfo> all_signals(1 + signal_count);

	queue_lock.Acquire();

	TimelinePoint timeline_point = graphics_semaphore.Signal();

	VkSemaphoreSubmitInfo timeline_signal_info = {};
	timeline_signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	timeline_signal_info.semaphore = timeline_point.semaphore;
	timeline_signal_info.value = timeline_point.frame;
	timeline_signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	all_signals[0] = timeline_signal_info;

	for (unsigned i = 0; i < signal_count; i++)
		all_signals[i + 1] = signals[i];

	VkCommandBufferSubmitInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 1;
	buffer_info.commandBuffer = cmd.GetVkHandle();

	VkSubmitInfo2 submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;

	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;

	submit_info.waitSemaphoreInfoCount = wait_count;
	submit_info.pWaitSemaphoreInfos = waits;

	submit_info.signalSemaphoreInfoCount = 1 + signal_count;
	submit_info.pSignalSemaphoreInfos = all_signals.data();

	VkResult submit_result = vkQueueSubmit2(graphics_queue.vk_handle, 1, &submit_info, VK_NULL_HANDLE);

	queue_lock.Release();

	GRAPHICS_VK_CHECK(submit_result, "Failed to submit command to queue.");

	return timeline_point;
}

gfx::CmdBuffer gfx::Device::SubmitImBegin()
{
	immediate_mutex.Lock();

	CmdBuffer cmd = immediate_pool->Acquire();
	cmd.Begin();

	return cmd;
}

void gfx::Device::SubmitImEnd(const CmdBuffer &cmd)
{
	ApplyBindlessUpdates();

	TimelinePoint point = Submit(cmd);
	WaitUntil(point);

	immediate_pool->Release(cmd, point.frame);
	immediate_pool->Purge(point.frame);

	immediate_mutex.Unlock();
}

VkQueryPool gfx::Device::QueryPoolCreate(unsigned query_count, VkQueryType type, VkQueryPipelineStatisticFlags pipeline_stat_flags)
{
	VkQueryPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	create_info.queryCount = query_count;
	create_info.queryType = type;
	create_info.pipelineStatistics = pipeline_stat_flags;

	VkQueryPool handle = VK_NULL_HANDLE;

	GRAPHICS_VK_CHECK(
		vkCreateQueryPool(vk_device,
			&create_info,
			nullptr,
			&handle
		),
		"Failed to create query pool."
	);

	return handle;
}

void gfx::Device::QueryPoolDestroy(VkQueryPool pool)
{
	vkDestroyQueryPool(vk_device, pool, nullptr);
}

void gfx::Device::WaitIdle()
{
	queue_lock.Acquire();
	vkDeviceWaitIdle(vk_device);
	queue_lock.Release();
}

void gfx::Device::WaitForFence(VkFence fence)
{
	vkWaitForFences(vk_device, 1, &fence, VK_TRUE, UINT64_MAX);
}

void gfx::Device::ResetFence(VkFence fence)
{
	vkResetFences(vk_device, 1, &fence);
}

void gfx::Device::DestroyFence(VkFence fence)
{
	vkDestroyFence(vk_device, fence, nullptr);
}

gfx::Semaphore gfx::Device::SemaphoreCreate(u64 value)
{
	VkSemaphoreTypeCreateInfo timeline_type_create_info = {};
	timeline_type_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	timeline_type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timeline_type_create_info.initialValue = value;

	VkSemaphoreCreateInfo timeline_semaphore_create_info = {};
	timeline_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	timeline_semaphore_create_info.flags = 0;
	timeline_semaphore_create_info.pNext = &timeline_type_create_info;

	VkSemaphore vk_semaphore = VK_NULL_HANDLE;

	GRAPHICS_VK_CHECK(
		vkCreateSemaphore(vk_device,
			&timeline_semaphore_create_info, nullptr,
			&vk_semaphore),
		"Failed to create timeline semaphore."
	);

	Semaphore semaphore;
	semaphore.vk_handle = vk_semaphore;
	semaphore.last_submitted_frame = value;

	return semaphore;
}

void gfx::Device::SemaphoreDestroy(const Semaphore &semaphore)
{
	vkDestroySemaphore(vk_device, semaphore.vk_handle, nullptr);
}

u64 gfx::Device::SemaphoreGPUCounterValue(const Semaphore &semaphore)
{
	u64 result = 0;

	vkGetSemaphoreCounterValue(
		vk_device,
		semaphore.vk_handle,
		&result
	);

	return result;
}

void gfx::Device::WaitUntil(TimelinePoint point)
{
	if (point.frame == 0)
		return;

	VkSemaphoreWaitInfo wait_info = {};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores = &point.semaphore;
	wait_info.pValues = &point.frame;

	GRAPHICS_VK_CHECK(
		vkWaitSemaphores(vk_device,
			&wait_info, UINT64_MAX
		),
		"Failed to wait on timeline semaphore."
	);
}

gfx::ResourceKey gfx::Device::PipelineLayoutFetch(ResourceKey program)
{
	ResourceKey pipeline_layout_key = program;

	if (PipelineLayoutFromKey(program))
		return pipeline_layout_key;

	const ShaderProgram *gfx_program = ShaderProgramFromKey(program);

	VkShaderStageFlags stage = VK_SHADER_STAGE_ALL_GRAPHICS;

	if (gfx_program->IsCompute())
		stage = VK_SHADER_STAGE_COMPUTE_BIT;

	VkPushConstantRange push_constants = {};
	push_constants.offset = 0;
	push_constants.size = gfx_program->push_constant_size;
	push_constants.stageFlags = stage;

	VkPipelineLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &bindless.layout;

	if (push_constants.size > 0) {
		create_info.pushConstantRangeCount = 1;
		create_info.pPushConstantRanges = &push_constants;
	} else {
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = nullptr;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	GRAPHICS_VK_CHECK(
		vkCreatePipelineLayout(vk_device,
			&create_info, nullptr,
			&layout
		),
		"Failed to create pipeline layout."
	);

	return pipeline_layouts.Push(layout, pipeline_layout_key);
}

VkPipelineLayout gfx::Device::PipelineLayoutFromKey(ResourceKey key)
{
	VkPipelineLayout *layout = pipeline_layouts.Get(key);
	return layout ? *layout : VK_NULL_HANDLE;
}

gfx::PipelineState gfx::Device::FetchGraphicsPipelineUnlocked(const GraphicsPipelineDef &def)
{
	ResourceKey hashed_key(hash::Struct(def));

	ResourceKey layout_key = PipelineLayoutFetch(def.program);

	if (PipelineFromKey(hashed_key)) {
		PipelineState st;
		st.pipeline = hashed_key;
		st.layout = layout_key;
		st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

		return st;
	}

	ShaderProgram *program = ShaderProgramFromKey(def.program);

	DebugLogAssert(log_channel, !program->IsCompute(), "Shader program must not be a compute shader!");

	VkPipelineLayout layout = PipelineLayoutFromKey(layout_key);

	static constexpr VkDynamicState graphics_pipeline_dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_state_create_info.pVertexBindingDescriptions = nullptr;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = def.topology;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = nullptr;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.f;
	rasterization_state_create_info.cullMode = def.cull_mode;
	rasterization_state_create_info.frontFace = def.front_face;
	rasterization_state_create_info.depthBiasEnable = def.depth_bias_enabled ? VK_TRUE : VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = def.depth_bias_constant;
	rasterization_state_create_info.depthBiasClamp = 0.f;
	rasterization_state_create_info.depthBiasSlopeFactor = def.depth_bias_slope;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = def.min_sample_shading_enabled;
	multisample_state_create_info.minSampleShading = def.min_sample_shading;
	multisample_state_create_info.rasterizationSamples = def.samples;
	multisample_state_create_info.pSampleMask = nullptr;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE; // todo: add option to turn this on
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	Vector<VkPipelineColorBlendAttachmentState> blend_states(def.colour_attachment_count);

	for (unsigned i = 0; i < def.colour_attachment_count; i++) {
		VkPipelineColorBlendAttachmentState *bs = &blend_states[i];

		*bs = {};

		bs->blendEnable = def.blend_state.enabled;

		bs->srcColorBlendFactor = def.blend_state.colour.src;
		bs->dstColorBlendFactor = def.blend_state.colour.dst;
		bs->colorBlendOp = def.blend_state.colour.op;

		bs->srcAlphaBlendFactor = def.blend_state.alpha.src;
		bs->dstAlphaBlendFactor = def.blend_state.alpha.dst;
		bs->alphaBlendOp = def.blend_state.alpha.op;

		if (def.blend_state.write_mask[0])
			bs->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (def.blend_state.write_mask[1])
			bs->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (def.blend_state.write_mask[2])
			bs->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (def.blend_state.write_mask[3])
			bs->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {};
	colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_create_info.logicOpEnable = def.blend_state.logic_op_enabled;
	colour_blend_state_create_info.logicOp = def.blend_state.logic_op;
	colour_blend_state_create_info.attachmentCount = def.colour_attachment_count;
	colour_blend_state_create_info.pAttachments = blend_states.data();
	colour_blend_state_create_info.blendConstants[0] = def.blend_state.constants[0];
	colour_blend_state_create_info.blendConstants[1] = def.blend_state.constants[1];
	colour_blend_state_create_info.blendConstants[2] = def.blend_state.constants[2];
	colour_blend_state_create_info.blendConstants[3] = def.blend_state.constants[3];

	const DepthStencilState &ds = def.depth_stencil_state;

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
	depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_state_create_info.depthTestEnable = ds.depth_test_enabled;
	depth_stencil_state_create_info.depthWriteEnable = ds.depth_write_enabled;
	depth_stencil_state_create_info.depthCompareOp = ds.depth_compare_op;
	depth_stencil_state_create_info.depthBoundsTestEnable = ds.depth_bounds_test_enabled;
	depth_stencil_state_create_info.minDepthBounds = ds.depth_bounds_min;
	depth_stencil_state_create_info.maxDepthBounds = ds.depth_bounds_max;
	depth_stencil_state_create_info.stencilTestEnable = ds.stencil_test_enabled;
	depth_stencil_state_create_info.front.failOp = ds.stencil_front.fail_op;
	depth_stencil_state_create_info.front.passOp = ds.stencil_front.pass_op;
	depth_stencil_state_create_info.front.depthFailOp = ds.stencil_front.depth_fail_op;
	depth_stencil_state_create_info.front.compareOp = ds.stencil_front.compare_op;
	depth_stencil_state_create_info.front.writeMask = ds.stencil_front.write_mask;
	depth_stencil_state_create_info.front.reference = ds.stencil_front.reference;
	depth_stencil_state_create_info.back.failOp = ds.stencil_back.fail_op;
	depth_stencil_state_create_info.back.passOp = ds.stencil_back.pass_op;
	depth_stencil_state_create_info.back.depthFailOp = ds.stencil_back.depth_fail_op;
	depth_stencil_state_create_info.back.compareOp = ds.stencil_back.compare_op;
	depth_stencil_state_create_info.back.writeMask = ds.stencil_back.write_mask;
	depth_stencil_state_create_info.back.reference = ds.stencil_back.reference;

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = ArraySize(graphics_pipeline_dynamic_states);
	dynamic_state_create_info.pDynamicStates = graphics_pipeline_dynamic_states;

	VkFormat depth_stencil_format = VK_FORMAT_UNDEFINED;

	if (def.has_depth_attachment) {
		depth_stencil_format = (def.depth_attachment_format != VK_FORMAT_UNDEFINED)
			? def.depth_attachment_format
			: GetDefaultDepthStencilFormat();
	}

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.viewMask = def.multi_view_mask;
	pipeline_rendering_create_info.colorAttachmentCount = def.colour_attachment_count;
	pipeline_rendering_create_info.pColorAttachmentFormats = def.colour_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat = depth_stencil_format;
	pipeline_rendering_create_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED; // passes never bind a stencil attachment

	VkShaderModuleCreateInfo module_infos[GRAPHICS_MAX_SHADER_STAGES] = {};
	VkPipelineShaderStageCreateInfo shader_stages[GRAPHICS_MAX_SHADER_STAGES] = {};

	for (unsigned i = 0; i < program->stage_count; i++) {
		module_infos[i].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_infos[i].codeSize = program->stages[i].bytecode.bytes.size();
		module_infos[i].pCode = (const unsigned *)program->stages[i].bytecode.bytes.data();
		module_infos[i].flags = 0;

		shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[i].stage = program->stages[i].flags;
		shader_stages[i].module = VK_NULL_HANDLE;
		shader_stages[i].pName = "main";
		shader_stages[i].pNext = &module_infos[i];
	}

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
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

	GRAPHICS_VK_CHECK(
		vkCreateGraphicsPipelines(vk_device,
			pipeline_process_cache,
			1, &graphics_pipeline_create_info,
			nullptr, &pipeline
		),
		"Failed to create graphics pipeline."
	);

	PipelineState st;
	st.pipeline = pipelines.Push(pipeline, hashed_key);
	st.layout = layout_key;
	st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	return st;
}

gfx::PipelineState gfx::Device::FetchGraphicsPipeline(const GraphicsPipelineDef &def)
{
	SpinLockScope scope(pipeline_cache_lock);
	return FetchGraphicsPipelineUnlocked(def);
}

gfx::PipelineState gfx::Device::FetchComputePipelineUnlocked(const ComputePipelineDef &def)
{
	ResourceKey hashed_key(hash::Struct(def));

	ResourceKey layout_key = PipelineLayoutFetch(def.program);

	if (PipelineFromKey(hashed_key)) {
		PipelineState st;
		st.pipeline = hashed_key;
		st.layout = layout_key;
		st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

		return st;
	}

	ShaderProgram *program = ShaderProgramFromKey(def.program);

	DebugLogAssert(log_channel, program->IsCompute(), "Shader program must be a compute shader!");

	VkPipelineLayout layout = PipelineLayoutFromKey(layout_key);

	VkShaderModuleCreateInfo module_info = {};
	module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_info.codeSize = program->stages[0].bytecode.bytes.size();
	module_info.pCode = (const unsigned *)program->stages[0].bytecode.bytes.data();
	module_info.flags = 0;

	VkPipelineShaderStageCreateInfo shader_stage = {};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage = program->stages[0].flags;
	shader_stage.module = VK_NULL_HANDLE;
	shader_stage.pName = "main";
	shader_stage.pNext = &module_info;

	VkComputePipelineCreateInfo compute_pipeline_create_info = {};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.stage = shader_stage;

	VkPipeline pipeline = VK_NULL_HANDLE;

	GRAPHICS_VK_CHECK(
		vkCreateComputePipelines(vk_device,
			pipeline_process_cache,
			1, &compute_pipeline_create_info,
			nullptr, &pipeline
		),
		"Failed to create compute pipeline."
	);

	PipelineState st;
	st.pipeline = pipelines.Push(pipeline, hashed_key);
	st.layout = layout_key;
	st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

	return st;
}

gfx::PipelineState gfx::Device::FetchComputePipeline(const ComputePipelineDef &def)
{
	SpinLockScope scope(pipeline_cache_lock);
	return FetchComputePipelineUnlocked(def);
}

VkPipeline gfx::Device::PipelineFromKey(ResourceKey key)
{
	VkPipeline *pipeline = pipelines.Get(key);
	return pipeline ? *pipeline : VK_NULL_HANDLE;
}

gfx::ResourceKey gfx::Device::TextureAlloc(const TextureAllocInfo &alloc_info)
{
	Texture texture;

	texture.width = alloc_info.width;
	texture.height = alloc_info.height;
	texture.depth = alloc_info.depth;

	texture.format = alloc_info.format;
	texture.type = alloc_info.type;
	texture.tiling = alloc_info.tiling;

	texture.mipmap_count = ClampMipmapCount(alloc_info.mipmaps, alloc_info.width, alloc_info.height, alloc_info.depth);
	texture.layer_count = alloc_info.layers;

	texture.sample_count = alloc_info.samples;

	texture.flags = 0;

	if (alloc_info.flags & TextureAllocFlag_Transient) {
		texture.flags |= TextureFlag_Transient;
		texture.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	} else {
		texture.usage =
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	// any depth format, not just the main depth-stencil one (shadow maps use D16/D32 without stencil).
	bool is_depth = TextureFormatIsDepth(alloc_info.format);
	bool is_cubemap = (alloc_info.flags & TextureAllocFlag_Cubemap) != 0;
	bool is_storage = (alloc_info.flags & TextureAllocFlag_Storage) != 0;

	if (is_depth) {
		texture.flags |= TextureFlag_Depth;
		texture.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		texture.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (TextureFormatHasStencil(alloc_info.format))
			texture.aspect_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
	} else {
		if (!TextureFormatIsBlockCompressed(alloc_info.format))
			texture.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // compressed images can't be attachments!
		texture.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageCreateFlags vk_create_flags = 0;
	VmaAllocationCreateFlags vma_alloc_flags = 0;

	if (is_cubemap) {
		texture.flags |= TextureFlag_Cubemap;
		vk_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	if (is_storage) {
		texture.flags |= TextureFlag_Storage;
		texture.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		vma_alloc_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	}

	VkImageCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = alloc_info.type;
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

	VmaAllocationCreateInfo vma_alloc_info = {};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.priority = 1.f;
	vma_alloc_info.flags = vma_alloc_flags;

	GRAPHICS_VK_CHECK(
		vmaCreateImage(vma_allocator,
			&create_info,
			&vma_alloc_info,
			&texture.vk_handle,
			&texture.allocation,
			&texture.allocation_info),
		"Failed to allocate texture."
	);

	return textures.PushAuto(texture);
}

gfx::ResourceKey gfx::Device::TextureAlloc2D(unsigned width, unsigned height, VkFormat format, unsigned mipmaps)
{
	TextureAllocInfo alloc_info;
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = TextureAllocFlag_None;

	return TextureAlloc(alloc_info);
}

gfx::ResourceKey gfx::Device::TextureAlloc2DRW(unsigned width, unsigned height, VkFormat format, unsigned mipmaps)
{
	TextureAllocInfo alloc_info;
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = TextureAllocFlag_Storage;

	return TextureAlloc(alloc_info);
}

gfx::ResourceKey gfx::Device::TextureAllocDepth2D(unsigned width, unsigned height, unsigned mipmaps)
{
	return TextureAlloc2D(width, height, GetDefaultDepthStencilFormat(), mipmaps);
}

gfx::ResourceKey gfx::Device::TextureAllocDepth2DRW(unsigned width, unsigned height, unsigned mipmaps)
{
	return TextureAlloc2DRW(width, height, GetDefaultDepthStencilFormat(), mipmaps);
}

gfx::ResourceKey gfx::Device::TextureAllocCubemap(unsigned resolution, VkFormat format, unsigned mipmaps)
{
	TextureAllocInfo alloc_info;
	alloc_info.width = resolution;
	alloc_info.height = resolution;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 6;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.flags = TextureAllocFlag_Cubemap;

	return TextureAlloc(alloc_info);
}

gfx::ResourceKey gfx::Device::TextureAllocCubemapDepth(unsigned resolution, unsigned mipmaps, VkFormat format)
{
	if (format == VK_FORMAT_UNDEFINED)
		format = GetDefaultDepthStencilFormat();

	return TextureAllocCubemap(resolution, format, mipmaps);
}

void gfx::Device::TextureDestroy(ResourceKey key)
{
	Texture *texture = TextureFromKey(key);
	DebugLogAssert(log_channel, texture, "Invalid texture with key %llu when destroying.", key.value);

	SpinLockScope scope(destroy_queue_lock);

	PendingDestroy &node = PushPendingDestroyLocked(PendingDestroyKind_Image, key);
	node.image = texture->vk_handle;
	node.allocation = texture->allocation;
}

gfx::Texture *gfx::Device::TextureFromKey(ResourceKey key)
{
	return textures.Get(key);
}

gfx::ResourceKey gfx::Device::TextureViewFetchUnlocked(const TextureViewCreateInfo &info)
{
	ResourceKey hashed_key(hash::Struct(info));

	if (TextureViewFromKey(hashed_key))
		return hashed_key;

	Texture *gfx_texture = TextureFromKey(info.texture);
	DebugLogAssert(log_channel, gfx_texture, "Invalid texture with key %llu when creating view.", info.texture.value);

	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = gfx_texture->vk_handle;
	view_create_info.viewType = info.type;
	view_create_info.format = gfx_texture->format;

	view_create_info.subresourceRange.aspectMask = info.range.aspects;
	view_create_info.subresourceRange.baseMipLevel = info.range.base_mip;
	view_create_info.subresourceRange.levelCount = info.range.mips;
	view_create_info.subresourceRange.baseArrayLayer = info.range.base_layer;
	view_create_info.subresourceRange.layerCount = info.range.layers;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	TextureView view;
	view.type = info.type;
	view.range = info.range;

	GRAPHICS_VK_CHECK(
		vkCreateImageView(vk_device,
			&view_create_info, nullptr,
			&view.vk_handle),
		"Failed to create texture image view."
	);

	bool is_cubemap = (gfx_texture->flags & TextureFlag_Cubemap) != 0;
	bool is_storage = (gfx_texture->flags & TextureFlag_Storage) != 0;

	view.bindless = bindless.RegisterView(view.vk_handle, is_cubemap, is_storage);

	return texture_views.Push(view, hashed_key);
}

gfx::ResourceKey gfx::Device::TextureViewFetch(const TextureViewCreateInfo &info)
{
	SpinLockScope scope(view_cache_lock);
	return TextureViewFetchUnlocked(info);
}

gfx::ResourceKey gfx::Device::TextureViewAuto(ResourceKey texture)
{
	Texture *gfx_texture = TextureFromKey(texture);
	DebugLogAssert(log_channel, gfx_texture, "Invalid texture with key %llu when creating view (auto).", texture.value);

	SubresourceRange range = {};
	range.aspects = gfx_texture->aspect_flags;
	range.base_mip = 0;
	range.mips = gfx_texture->mipmap_count;
	range.base_layer = 0;
	range.layers = gfx_texture->layer_count;

	VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;

	if (gfx_texture->flags & TextureFlag_Cubemap)
		view_type = VK_IMAGE_VIEW_TYPE_CUBE;

	TextureViewCreateInfo info;
	info.texture = texture;
	info.type = view_type;
	info.range = range;

	return TextureViewFetch(info);
}

gfx::TextureView *gfx::Device::TextureViewFromKey(ResourceKey key)
{
	return texture_views.Get(key);
}

unsigned gfx::Device::TextureViewBindless(ResourceKey key)
{
	TextureView *gfx_view = TextureViewFromKey(key);
	DebugLogAssert(log_channel, gfx_view, "Invalid texture view with key %llu when getting bindless info.", key.value);

	return gfx_view->bindless;
}

gfx::ResourceKey gfx::Device::BufferAlloc(const BufferAllocInfo &alloc_info)
{
	Buffer buffer;
	buffer.usage = alloc_info.usage;
	buffer.size = alloc_info.size;
	buffer.allocation_flags = alloc_info.flags;

	// just implicitly make all storage buffers have device address
	// todo: is this a good idea?
	if ((buffer.usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0)
		buffer.usage |= VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferUsageFlags2CreateInfo usage_info = {};
	usage_info.sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO;
	usage_info.usage = buffer.usage;

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = &usage_info;
	buffer_create_info.size = buffer.size;
	buffer_create_info.usage = 0;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = nullptr;

	VmaAllocationCreateInfo vma_alloc_info = {};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = alloc_info.flags;

	// persistently map buffers we wanna be writing into cause it's more convenient and the performance loss is negligible
	if (alloc_info.flags & (VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT))
		vma_alloc_info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;

	GRAPHICS_VK_CHECK(
		vmaCreateBuffer(vma_allocator,
			&buffer_create_info,
			&vma_alloc_info,
			&buffer.vk_handle,
			&buffer.allocation,
			&buffer.allocation_info),
		"Failed to allocate buffer."
	);

	if ((buffer.usage & VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT) != 0) {
		VkBufferDeviceAddressInfo address_info = {};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer.vk_handle;

		buffer.device_address = vkGetBufferDeviceAddress(vk_device, &address_info);
	}

	return buffers.PushAuto(buffer);
}

gfx::ResourceKey gfx::Device::StageAlloc(u64 size)
{
	BufferAllocInfo alloc_info;
	alloc_info.usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
	alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	alloc_info.size = size;

	return BufferAlloc(alloc_info);
}

void gfx::Device::BufferDestroy(ResourceKey key)
{
	Buffer *buffer = BufferFromKey(key);
	DebugLogAssert(log_channel, buffer, "Invalid buffer with key %llu when destroying.", key.value);

	SpinLockScope scope(destroy_queue_lock);

	PendingDestroy &node = PushPendingDestroyLocked(PendingDestroyKind_Buffer, key);
	node.buffer = buffer->vk_handle;
	node.allocation = buffer->allocation;
}

gfx::Buffer *gfx::Device::BufferFromKey(ResourceKey key)
{
	return buffers.Get(key);
}

void *gfx::Device::BufferMap(ResourceKey key)
{
	Buffer *buffer = BufferFromKey(key);
	DebugLogAssert(log_channel, buffer, "Invalid buffer with key %llu when mapping memory.", key.value);

	return buffer->allocation_info.pMappedData;
}

u64 gfx::Device::BufferAddress(ResourceKey key)
{
	Buffer *buffer = BufferFromKey(key);
	DebugLogAssert(log_channel, buffer, "Invalid buffer with key %llu when getting device address.", key.value);

	return buffer->device_address;
}

void gfx::Device::BufferRead(ResourceKey key, void *dst, u64 length, u64 offset)
{
	Buffer *buffer = BufferFromKey(key);
	DebugLogAssert(log_channel, buffer, "Invalid buffer with key %llu when reading.", key.value);

	vmaCopyAllocationToMemory(vma_allocator, buffer->allocation, offset, dst, length);
}

void gfx::Device::BufferWrite(ResourceKey key, const void *src, u64 length, u64 offset)
{
	Buffer *buffer = BufferFromKey(key);
	DebugLogAssert(log_channel, buffer, "Invalid buffer with key %llu when writing.", key.value);

	vmaCopyMemoryToAllocation(vma_allocator, src, buffer->allocation, offset, length);
}

u64 gfx::Device::BufferSize(ResourceKey key)
{
	Buffer *buffer = BufferFromKey(key);
	DebugLogAssert(log_channel, buffer, "Invalid buffer with key %llu when getting size.", key.value);

	return buffer->size;
}

gfx::ResourceKey gfx::Device::SamplerCreate(const SamplerCreateInfo &info)
{
	const VkPhysicalDeviceProperties &properties = vk_physical_device_properties.properties;

	VkSamplerCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.minFilter = info.filter;
	create_info.magFilter = info.filter;
	create_info.addressModeU = info.wrap_x;
	create_info.addressModeV = info.wrap_y;
	create_info.addressModeW = info.wrap_z;
	create_info.anisotropyEnable = VK_FALSE;
	create_info.maxAnisotropy = 1.f;

	if (info.anisotropy) {
		create_info.anisotropyEnable = VK_TRUE;
		create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	}

	create_info.borderColor = info.border_colour;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = info.compare ? VK_TRUE : VK_FALSE;
	create_info.compareOp = info.compare ? info.compare_op : VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.f;
	create_info.minLod = 0.f;
	create_info.maxLod = info.max_lod;

	Sampler sampler;

	GRAPHICS_VK_CHECK(
		vkCreateSampler(vk_device,
			&create_info, nullptr,
			&sampler.vk_handle
		),
		"Failed to create texture sampler."
	);

	sampler.filter = info.filter;
	sampler.wrap_x = info.wrap_x;
	sampler.wrap_y = info.wrap_y;
	sampler.wrap_z = info.wrap_z;
	sampler.border_colour = info.border_colour;
	sampler.bindless = bindless.RegisterSampler(sampler.vk_handle);

	return samplers.PushAuto(sampler);
}

gfx::ResourceKey gfx::Device::SamplerCreateF(VkFilter filter)
{
	SamplerCreateInfo create_info;
	create_info.filter = filter;
	create_info.wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.border_colour = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	return SamplerCreate(create_info);
}

void gfx::Device::SamplerDestroy(ResourceKey key)
{
	Sampler *sampler = SamplerFromKey(key);
	DebugLogAssert(log_channel, sampler, "Invalid sampler with key %llu when destroying.", key.value);

	SpinLockScope scope(destroy_queue_lock);

	PendingDestroy &node = PushPendingDestroyLocked(PendingDestroyKind_Sampler, key);
	node.sampler = sampler->vk_handle;
	node.sampler_bindless = sampler->bindless;
}

gfx::Sampler *gfx::Device::SamplerFromKey(ResourceKey key)
{
	return samplers.Get(key);
}

unsigned gfx::Device::SamplerBindless(ResourceKey key)
{
	Sampler *gfx_sampler = SamplerFromKey(key);
	DebugLogAssert(log_channel, gfx_sampler, "Invalid sampler with key %llu when getting bindless info.", key.value);

	return gfx_sampler->bindless;
}

gfx::ShaderStage gfx::Device::ShaderStageCreate(const ShaderBytecode &bytecode)
{
	SpvReflectShaderModule reflect_module = {};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(bytecode.bytes.size(), bytecode.bytes.data(), &reflect_module);

	DebugLogAssert(
		log_channel,
		reflect_result == SPV_REFLECT_RESULT_SUCCESS,
		"Failed to reflect SPIR-V module: %d\n", reflect_result
	);

	ShaderStage stage;

	if (reflect_module.entry_point_count >= 1) {
		stage.flags = (VkShaderStageFlagBits)reflect_module.entry_points[0].shader_stage;

		unsigned push_constant_count = 0;
		reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, nullptr);

		if (reflect_result == SPV_REFLECT_RESULT_SUCCESS && push_constant_count > 0) {
			Vector<SpvReflectBlockVariable *> pcs(push_constant_count);

			spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, pcs.data());

			for (unsigned i = 0; i < push_constant_count; i++) {
				SpvReflectBlockVariable *pc = pcs[i];

				unsigned alignment = 4;

				for (unsigned j = 0; j < pc->member_count; j++)
					alignment = maths::Max(alignment, pc->members[j].size);

				unsigned padded = MemAlignUp(pc->size, alignment);

				stage.push_constant_size = maths::Max(stage.push_constant_size, padded);
			}
		}

		stage.bytecode = bytecode;
	} else {
		DebugLogB(log_channel, "No entry points found in SPIR-V.");
	}

	spvReflectDestroyShaderModule(&reflect_module);

	return stage;
}

gfx::ResourceKey gfx::Device::ShaderProgramCreate(unsigned stage_count, const ShaderBytecode *stages)
{
	ShaderProgram program;
	program.stage_count = stage_count;
	program.push_constant_size = 0;

	for (unsigned i = 0; i < stage_count; i++) {
		program.stages[i] = ShaderStageCreate(stages[i]);
		program.push_constant_size = maths::Max(program.push_constant_size, program.stages[i].push_constant_size);
	}

	return shaders.PushAuto(program);
}

void gfx::Device::ShaderProgramDestroy(ResourceKey key)
{
	ShaderProgram *shader = ShaderProgramFromKey(key);
	DebugLogAssert(log_channel, shader, "Invalid shader program with key %llu when destroying.", (u64)key.value);

	SpinLockScope scope(destroy_queue_lock);

	PushPendingDestroyLocked(PendingDestroyKind_Shader, key);
}

gfx::ShaderProgram *gfx::Device::ShaderProgramFromKey(ResourceKey key)
{
	return shaders.Get(key);
}

gfx::AccelStructReceipt gfx::Device::BLASAlloc(const BLASGeometry *geometries, unsigned geometry_count)
{
	Vector<VkAccelerationStructureGeometryKHR> vk_geometries(geometry_count);
	Vector<unsigned> primitive_counts(geometry_count);

	for (unsigned i = 0; i < geometry_count; i++) {
		const BLASGeometry *geometry = &geometries[i];

		Buffer *vb = BufferFromKey(geometry->vertex_buffer);
		Buffer *ib = BufferFromKey(geometry->index_buffer);

		vk_geometries[i] = {};
		vk_geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vk_geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vk_geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureGeometryTrianglesDataKHR *tri = &vk_geometries[i].geometry.triangles;

		tri->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		tri->vertexFormat = geometry->vertex_format;
		tri->vertexData.deviceAddress = vb->device_address + geometry->vertex_offset;
		tri->vertexStride = geometry->vertex_stride;
		tri->maxVertex = geometry->vertex_count - 1;

		tri->indexType = IndexTypeToVk(geometry->index_type);
		tri->indexData.deviceAddress = ib->device_address + geometry->index_offset;

		primitive_counts[i] = geometry->index_count / 3;
	}

	// so much fucking typing oh my god.

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.geometryCount = geometry_count;
	build_info.pGeometries = vk_geometries.data();

	VkAccelerationStructureBuildSizesInfoKHR sizes = {};
	sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	vkGetAccelerationStructureBuildSizesKHR(
		vk_device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&build_info,
		primitive_counts.data(),
		&sizes
	);

	// backing buffer also needs alloc

	// this feels bad 'cuz we should be automatically generating the device address
	// in the buffer alloc like we do for usual storage buffers but i figure if
	// this is the only time we do this i can get away with being lazy :p
	BufferAllocInfo buf_alloc_info;
	buf_alloc_info.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
	buf_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buf_alloc_info.size = sizes.accelerationStructureSize;

	ResourceKey backing_buffer_key = BufferAlloc(buf_alloc_info);
	Buffer *backing_buffer = BufferFromKey(backing_buffer_key);

	VkAccelerationStructureCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.buffer = backing_buffer->vk_handle;
	create_info.size = sizes.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	AccelStruct accel_struct;
	accel_struct.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accel_struct.backing_buffer = backing_buffer_key;

	GRAPHICS_VK_CHECK(
		vkCreateAccelerationStructureKHR(vk_device,
			&create_info, nullptr,
			&accel_struct.vk_handle),
		"Failed to create BLAS."
	);

	VkAccelerationStructureDeviceAddressInfoKHR addr_info = {}; // fuck me khronos shorter names would be appreciated yes??
	addr_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addr_info.accelerationStructure = accel_struct.vk_handle;

	accel_struct.device_address = vkGetAccelerationStructureDeviceAddressKHR(vk_device, &addr_info);

	DebugLogD(log_channel, "Allocated BLAS from %u geometries.", geometry_count);

	AccelStructReceipt receipt;
	receipt.key = accel_structures.PushAuto(accel_struct);
	receipt.scratch_size = sizes.buildScratchSize;

	return receipt;
}

gfx::AccelStructReceipt gfx::Device::TLASAlloc(unsigned max_instance_count)
{
	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR; // fuck you

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.geometryCount = 1;
	build_info.pGeometries = &geometry;

	VkAccelerationStructureBuildSizesInfoKHR sizes = {};
	sizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	vkGetAccelerationStructureBuildSizesKHR(
		vk_device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&build_info,
		&max_instance_count,
		&sizes
	);

	// backing buffer also needs alloc

	// this feels bad cuz' we should be automatically generating the device address
	// in the buffer alloc like we do for usual storage buffers but i figure if
	// this is the only time we do this i can get away with being lazy :p
	BufferAllocInfo buf_alloc_info;
	buf_alloc_info.usage = VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
	buf_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buf_alloc_info.size = sizes.accelerationStructureSize;

	ResourceKey backing_buffer_key = BufferAlloc(buf_alloc_info);
	Buffer *backing_buffer = BufferFromKey(backing_buffer_key);

	VkAccelerationStructureCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	create_info.buffer = backing_buffer->vk_handle;
	create_info.size = sizes.accelerationStructureSize;
	create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	AccelStruct accel_struct;
	accel_struct.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accel_struct.backing_buffer = backing_buffer_key;

	GRAPHICS_VK_CHECK(
		vkCreateAccelerationStructureKHR(vk_device,
			&create_info, nullptr,
			&accel_struct.vk_handle
		),
		"Failed to create TLAS."
	);

	VkAccelerationStructureDeviceAddressInfoKHR addr_info = {}; // fuck me khronos shorter names would be appreciated yes??
	addr_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	addr_info.accelerationStructure = accel_struct.vk_handle;

	accel_struct.device_address = vkGetAccelerationStructureDeviceAddressKHR(vk_device, &addr_info);

	DebugLogD(log_channel, "Allocated TLAS with max instance count of %u.", max_instance_count);

	AccelStructReceipt receipt;
	receipt.key = accel_structures.PushAuto(accel_struct);
	receipt.scratch_size = sizes.buildScratchSize;

	return receipt;
}

void gfx::Device::AccelStructDestroy(ResourceKey key)
{
	AccelStruct *accel_struct = AccelStructFromKey(key);
	DebugLogAssert(log_channel, accel_struct, "Invalid acceleration structure with key %llu when destroying.", key.value);

	BufferDestroy(accel_struct->backing_buffer);

	SpinLockScope scope(destroy_queue_lock);

	PendingDestroy &node = PushPendingDestroyLocked(PendingDestroyKind_AccelStruct, key);
	node.accel = accel_struct->vk_handle;
}

u64 gfx::Device::AccelStructAddress(ResourceKey key)
{
	AccelStruct *accel_struct = AccelStructFromKey(key);
	DebugLogAssert(log_channel, accel_struct, "Invalid acceleration structure with key %llu when getting device address.", key.value);

	return accel_struct->device_address;
}

gfx::AccelStruct *gfx::Device::AccelStructFromKey(ResourceKey key)
{
	return accel_structures.Get(key);
}

void gfx::Device::CreateSyncResources()
{
	graphics_semaphore = SemaphoreCreate(0);

	unsigned family_index = graphics_queue.family_index;

	for (unsigned i = 0; i < GRAPHICS_FRAMES_IN_FLIGHT; i++) {
		FrameInFlight *frame = &frames_in_flight[i];

		frame->completion_point.frame = 0;
		frame->completion_point.semaphore = graphics_semaphore.vk_handle;

		frame->command_pool = new CmdPool(vk_device, family_index, log_channel);

		// semaphore
		{
			VkSemaphoreCreateInfo binary_semaphore_create_info = {};
			binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			binary_semaphore_create_info.flags = 0;
			binary_semaphore_create_info.pNext = nullptr;

			GRAPHICS_VK_CHECK(
				vkCreateSemaphore(vk_device,
					&binary_semaphore_create_info, nullptr,
					&frame->image_available_semaphore),
				"Failed to create image available semaphore."
			);
		}

		frame->frame_number = 0;
	}

	immediate_pool = new CmdPool(vk_device, family_index, log_channel);

	DebugLogD(log_channel, "Created frame sync objects.");
}

void gfx::Device::DestroySyncResources()
{
	for (unsigned i = 0; i < GRAPHICS_FRAMES_IN_FLIGHT; i++) {
		FrameInFlight *frame = &frames_in_flight[i];

		vkDestroySemaphore(vk_device, frame->image_available_semaphore, nullptr);

		delete frame->command_pool;
		frame->command_pool = nullptr;
	}

	delete immediate_pool;
	immediate_pool = nullptr;

	SemaphoreDestroy(graphics_semaphore);
}

void gfx::Device::CreateBindless()
{
	VkDescriptorPoolSize pool_sizes[BindlessKind_MaxEnum] = {};

	for (unsigned i = 0; i < BindlessKind_MaxEnum; i++) {
		pool_sizes[i].type = BindlessKindToVk((BindlessKind)i);
		pool_sizes[i].descriptorCount = GRAPHICS_BINDLESS_MAX_RESOURCES;
	}

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = ArraySize(pool_sizes) * GRAPHICS_BINDLESS_MAX_RESOURCES;
	pool_create_info.poolSizeCount = ArraySize(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;

	GRAPHICS_VK_CHECK(
		vkCreateDescriptorPool(vk_device,
			&pool_create_info, nullptr,
			&bindless.pool),
		"Failed to create bindless descriptor pool."
	);

	VkDescriptorSetLayoutBinding bindings[BindlessKind_MaxEnum] = {};
	VkDescriptorBindingFlags bindless_flags[BindlessKind_MaxEnum] = {};

	for (unsigned i = 0; i < BindlessKind_MaxEnum; i++) {
		bindings[i].descriptorType = BindlessKindToVk((BindlessKind)i);
		bindings[i].descriptorCount = GRAPHICS_BINDLESS_MAX_RESOURCES;
		bindings[i].binding = i;
		bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
		bindings[i].pImmutableSamplers = nullptr;

		bindless_flags[i] =
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = ArraySize(bindless_flags);
	binding_flags.pBindingFlags = bindless_flags;

	VkDescriptorSetLayoutCreateInfo layout_create_info = {};
	layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_create_info.bindingCount = ArraySize(bindings);
	layout_create_info.pBindings = bindings;
	layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
	layout_create_info.pNext = &binding_flags;

	GRAPHICS_VK_CHECK(
		vkCreateDescriptorSetLayout(vk_device,
			&layout_create_info, nullptr,
			&bindless.layout),
		"Failed to create bindless descriptor layout."
	);

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = bindless.pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &bindless.layout;

	GRAPHICS_VK_CHECK(
		vkAllocateDescriptorSets(vk_device,
			&alloc_info,
			&bindless.set),
		"Failed to allocate bindless descriptor set."
	);

	DebugLogD(log_channel, "Bindless resources created.");
}

void gfx::Device::DestroyBindless()
{
	vkDestroyDescriptorSetLayout(vk_device, bindless.layout, nullptr);
	vkDestroyDescriptorPool(vk_device, bindless.pool, nullptr);
}

void gfx::Device::ApplyBindlessUpdates()
{
	static thread_local BindlessUpdate updates[512];

	SpinLockScope scope(bindless_apply_lock);

	unsigned count = bindless.DrainUpdatesInto(updates, 512);

	if (count == 0)
		return;

	Vector<VkWriteDescriptorSet> writes(count);
	Vector<VkDescriptorImageInfo> infos(count);

	for (unsigned i = 0; i < count; i++) {
		BindlessUpdate *update = &updates[i];

		infos[i].sampler = update->sampler;
		infos[i].imageView = update->view;
		infos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		writes[i] = {};
		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].descriptorCount = 1;
		writes[i].dstArrayElement = update->slot;
		writes[i].descriptorType = BindlessKindToVk(update->kind);
		writes[i].dstSet = bindless.set;
		writes[i].dstBinding = update->kind;
		writes[i].pImageInfo = &infos[i];
	}

	vkUpdateDescriptorSets(
		vk_device,
		count, writes.data(),
		0, nullptr
	);
}

void gfx::Device::CreateImGui()
{
	if (!ImGui::GetCurrentContext())
		return;

	const unsigned max_sets = 1000;

	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, max_sets },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_sets },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, max_sets },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, max_sets }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = max_sets;
	pool_info.poolSizeCount = ArraySize(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	GRAPHICS_VK_CHECK(
		vkCreateDescriptorPool(vk_device,
			&pool_info, nullptr,
			&imgui_pool),
		"Failed to create ImGui descriptor pool."
	);

	// The UI is drawn straight onto the swapchain image.
	static VkFormat swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.ApiVersion = MAGPIE_VULKAN_API_VERSION;
	init_info.Instance = vk_instance;
	init_info.PhysicalDevice = vk_physical_device;
	init_info.Device = vk_device;
	init_info.QueueFamily = graphics_queue.family_index;
	init_info.Queue = graphics_queue.vk_handle;
	init_info.PipelineCache = pipeline_process_cache;
	init_info.DescriptorPool = imgui_pool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = GRAPHICS_FRAMES_IN_FLIGHT;
	init_info.ImageCount = GRAPHICS_FRAMES_IN_FLIGHT;
	init_info.CheckVkResultFn = nullptr;
	init_info.UseDynamicRendering = true;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_image_format;

	ImGui_ImplVulkan_Init(&init_info);

	imgui_enabled = true;
}

void gfx::Device::DestroyImGui()
{
	if (!imgui_enabled)
		return;

	imgui_textures.clear();

	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(vk_device, imgui_pool, nullptr);

	imgui_enabled = false;
}

void gfx::Device::ImGuiNewFrame()
{
	if (imgui_enabled)
		ImGui_ImplVulkan_NewFrame();
}

u64 gfx::Device::ImGuiTextureIdForView(ResourceKey view_key)
{
	if (!imgui_enabled)
		return 0;

	auto it = imgui_textures.find(view_key.value);

	if (it != imgui_textures.end())
		return (u64)it->second;

	TextureView *view = TextureViewFromKey(view_key);

	if (!view)
		return 0;

	VkDescriptorSet set = ImGui_ImplVulkan_AddTexture(view->vk_handle, VK_IMAGE_LAYOUT_GENERAL);
	imgui_textures[view_key.value] = set;

	return (u64)set;
}

void gfx::Device::ImGuiForgetTextureView(ResourceKey view_key)
{
	auto it = imgui_textures.find(view_key.value);

	if (it == imgui_textures.end())
		return;

	SpinLockScope scope(destroy_queue_lock);

	PendingDestroy &node = PushPendingDestroyLocked(PendingDestroyKind_ImGuiTexture, view_key);
	node.imgui_set = it->second;

	imgui_textures.erase(it);
}

void gfx::Device::ImGuiRecord(const CmdBuffer &cmd) const
{
	if (!imgui_enabled)
		return;

	ImDrawData *draw_data = ImGui::GetDrawData();

	if (draw_data)
		ImGui_ImplVulkan_RenderDrawData(draw_data, cmd.GetVkHandle());
}

gfx::Swapchain::Swapchain(Device *device_)
	: vk_handle(VK_NULL_HANDLE)
	, current_frame_index(0)
	, width(0)
	, height(0)
	, format(VK_FORMAT_UNDEFINED)
	, device(device_)
{
	const SwapchainSupportDetails *details = &device->swapchain_details;

	VkSurfaceFormatKHR surface_format = ChooseSwapchainSurfaceFormat(device->log_channel, device->vk_physical_device, details->surface_formats);
	VkPresentModeKHR present_mode = ChooseSwapchainPresentMode(details->present_modes, false); // TODO: add option to enable VSYNC
	VkExtent2D extent = ChooseSwapchainExtent(device->window, details->capabilities);

	width = extent.width;
	height = extent.height;
	format = surface_format.format;

	needs_manual_srgb_encode =
		format != VK_FORMAT_B8G8R8A8_SRGB &&
		format != VK_FORMAT_R8G8B8A8_SRGB &&
		format != VK_FORMAT_A8B8G8R8_SRGB_PACK32;

	unsigned frame_count = details->capabilities.minImageCount + 1;

	if (frame_count > details->capabilities.maxImageCount && details->capabilities.maxImageCount > 0)
		frame_count = details->capabilities.maxImageCount;

	DebugLogD(device->log_channel, "Swapchain:");
	DebugLogD(device->log_channel, " size: %u x %u", extent.width, extent.height);
	DebugLogD(device->log_channel, " format: %u", surface_format.format);
	DebugLogD(device->log_channel, " frame count: %u", frame_count);

	const VkImageUsageFlags swapchain_texture_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = device->vk_surface;
	create_info.minImageCount = frame_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = swapchain_texture_usage;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = nullptr;
	create_info.preTransform = details->capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	GRAPHICS_VK_CHECK(
		vkCreateSwapchainKHR(device->vk_device,
			&create_info, nullptr,
			&vk_handle
		),
		"Failed to create swapchain."
	);

	vkGetSwapchainImagesKHR(device->vk_device, vk_handle, &frame_count, nullptr);

	DebugLogAssert(device->log_channel, frame_count > 0, "Failed to find any images in swapchain.");

	Vector<VkImage> vk_images(frame_count);

	vkGetSwapchainImagesKHR(device->vk_device, vk_handle, &frame_count, vk_images.data());

	frames.resize(frame_count);

	for (unsigned i = 0; i < frame_count; i++) {
		SwapchainFrame *frame = &frames[i];

		// texture
		{
			Texture texture;
			texture.vk_handle = vk_images[i];
			texture.width = width;
			texture.height = height;
			texture.depth = 1;
			texture.flags = TextureFlag_Swapchain;
			texture.format = format;
			texture.type = VK_IMAGE_TYPE_2D;
			texture.tiling = VK_IMAGE_TILING_OPTIMAL;
			texture.usage = swapchain_texture_usage;
			texture.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
			texture.layer_count = 1;
			texture.mipmap_count = 1;
			texture.sample_count = VK_SAMPLE_COUNT_1_BIT;

			frame->texture_key = device->textures.PushAuto(texture);
		}

		// view
		{
			frame->texture_view.type = VK_IMAGE_VIEW_TYPE_2D;
			frame->texture_view.range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
			frame->texture_view.range.base_mip = 0;
			frame->texture_view.range.mips = 1;
			frame->texture_view.range.base_layer = 0;
			frame->texture_view.range.layers = 1;

			VkImageViewCreateInfo view_create_info = {};
			view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_create_info.image = vk_images[i];
			view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			view_create_info.format = format;

			view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_create_info.subresourceRange.baseMipLevel = 0;
			view_create_info.subresourceRange.levelCount = 1;
			view_create_info.subresourceRange.baseArrayLayer = 0;
			view_create_info.subresourceRange.layerCount = 1;

			view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			GRAPHICS_VK_CHECK(
				vkCreateImageView(device->vk_device,
					&view_create_info, nullptr,
					&frame->texture_view.vk_handle),
				"Failed to create texture image view."
			);
		}

		// semaphore
		{
			VkSemaphoreCreateInfo binary_semaphore_create_info = {};
			binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			binary_semaphore_create_info.flags = 0;
			binary_semaphore_create_info.pNext = nullptr;

			GRAPHICS_VK_CHECK(
				vkCreateSemaphore(device->vk_device,
					&binary_semaphore_create_info, nullptr,
					&frame->render_finished_semaphore),
				"Failed to create render finished semaphore."
			);
		}
	}

	DebugLogD(device->log_channel, "Swapchain created.");
}

gfx::Swapchain::~Swapchain()
{
	for (const SwapchainFrame &frame : frames) {
		vkDestroyImageView(device->vk_device, frame.texture_view.vk_handle, nullptr);
		vkDestroySemaphore(device->vk_device, frame.render_finished_semaphore, nullptr);

		device->textures.Return(frame.texture_key);
	}

	vkDestroySwapchainKHR(device->vk_device, vk_handle, nullptr);
}
