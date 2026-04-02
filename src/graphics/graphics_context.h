#ifndef GRAPHICS_CONTEXT_H
#define GRAPHICS_CONTEXT_H

typedef struct GFX_SwapchainSupportDetails GFX_SwapchainSupportDetails;
struct GFX_SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	u32 surface_format_count;
	VkSurfaceFormatKHR surface_formats[8];
	u32 present_mode_count;
	VkPresentModeKHR present_modes[8];
};

typedef struct GFX_HardwareQueue GFX_HardwareQueue;
struct GFX_HardwareQueue
{
	VkQueue handle;
	u32 family_index;
};

b32 GFX_HardwareQueueIsValid(const GFX_HardwareQueue *hq);

typedef struct GFX_Context GFX_Context;
struct GFX_Context
{
	VkInstance instance;
	VkDevice device;

	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties2 physical_device_properties;
	VkPhysicalDeviceFeatures2 physical_device_features;
	
	VkSurfaceKHR surface;

	VmaAllocator vma_allocator;

	GFX_HardwareQueue graphics_queue;

	VkPipelineCache pipeline_process_cache;
	
	b32 has_validation_layers;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkFormat depth_format;
	VkSampleCountFlagBits max_msaa_samples;

	GFX_SwapchainSupportDetails swapchain_details;
};

/* ==================================================
   INTERNALS
   ================================================== */

internal VkFormat GFX_ContextFindGraphicsSupportedFormat(VkPhysicalDevice physical_device,
														 VkImageTiling tiling,
														 VkFormatFeatureFlags features,
														 u32 candidate_count, const VkFormat *candidates);

internal VkFormat GFX_ContextFindGraphicsDepthFormat(VkPhysicalDevice physical_device);

internal VkSampleCountFlagBits GFX_ContextFindGraphicsMaxUsableSampleCount(VkPhysicalDeviceProperties2 properties);

internal const char *const *GFX_ContextGetInstanceExtensions(Arena *arena, u32 *extension_count);

internal b32 GFX_ContextCheckGraphicsPhysicalDeviceExtensionSupport(VkPhysicalDevice physical_device);

internal b32 GFX_ContextCheckForValidationLayerSupport(void);

internal GFX_SwapchainSupportDetails GFX_ContextQuerySwapchainSupport(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

internal u32 GFX_ContextAssignGraphicsPhysicalDeviceUsability(VkSurfaceKHR surface,
															  VkPhysicalDevice physical_device,
															  VkPhysicalDeviceProperties2 properties,
															  VkPhysicalDeviceFeatures2 features,
															  b32 *has_essentials);


internal VKAPI_ATTR VkBool32 VKAPI_CALL GFX_ContextVulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
																	   VkDebugUtilsMessageTypeFlagsEXT message_type,
																	   const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
																	   void *user_data);

internal VkResult GFX_ContextCreateDeviceDebugUtilsMessengerExt(VkInstance instance,
																 VkDebugUtilsMessengerCreateInfoEXT *debug_info,
																 const VkAllocationCallbacks *allocator,
																 VkDebugUtilsMessengerEXT *messenger);

/* ==================================================
   CONTEXT CORE
   ================================================== */

internal GFX_Context GFX_ContextInit();
internal void GFX_ContextDestroy(GFX_Context *context);

#endif // GRAPHICS_CONTEXT_H
