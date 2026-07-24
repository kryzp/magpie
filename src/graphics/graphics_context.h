#ifndef GRAPHICS_CONTEXT_H
#define GRAPHICS_CONTEXT_H

typedef struct G_SwapchainSupportDetails G_SwapchainSupportDetails;
struct G_SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	u32 surface_format_count;
	VkSurfaceFormatKHR *surface_formats;

	u32 present_mode_count;
	VkPresentModeKHR *present_modes;
};

typedef struct G_HardwareQueue G_HardwareQueue;
struct G_HardwareQueue
{
	VkQueue vk_handle;
	u32 family_index;
};

b32 G_HardwareQueueIsValid(const G_HardwareQueue *hq);

// todo: make some kinda limits struct / expose ?

typedef struct G_Context G_Context;
struct G_Context
{
	VkInstance instance;
	VkDevice device;

	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties2 physical_device_properties;
	VkPhysicalDeviceFeatures2 physical_device_features;

	G_ResolvedCapabilities capabilities;
	
	VkSurfaceKHR surface;

	VmaAllocator vma_allocator;

	G_HardwareQueue graphics_queue;

	VkPipelineCache pipeline_process_cache;
	
	b32 has_validation_layers;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkFormat depth_format;
	VkSampleCountFlagBits max_msaa_samples;
	u64 max_push_constants_size; // todo: move into a seperate limits struct

	G_SwapchainSupportDetails swapchain_details;
};

/* ==================================================
   INTERNALS
   ================================================== */

internal VkFormat G_ContextFindGraphicsSupportedFormat(VkPhysicalDevice physical_device,
													   VkImageTiling tiling,
													   VkFormatFeatureFlags features,
													   u32 candidate_count, const VkFormat *candidates,
													   LOG_Channel log_channel);

internal VkFormat G_ContextFindGraphicsDepthFormat(VkPhysicalDevice physical_device,
												   LOG_Channel log_channel);

internal VkSampleCountFlagBits G_ContextFindGraphicsMaxUsableSampleCount(VkPhysicalDeviceProperties2 properties,
																		 LOG_Channel log_channel);

internal const char * const *G_ContextGetInstanceExtensions(Arena *arena,
															u32 *extension_count,
															LOG_Channel log_channel);

internal b32 G_ContextCheckForValidationLayerSupport(void);

internal G_SwapchainSupportDetails G_ContextQuerySwapchainSupport(Arena *arena,
																  VkPhysicalDevice physical_device,
																  VkSurfaceKHR surface);

internal VKAPI_ATTR VkBool32 VKAPI_CALL G_ContextVulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
																	 VkDebugUtilsMessageTypeFlagsEXT message_type,
																	 const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
																	 void *user_data);

internal VkResult G_ContextCreateDeviceDebugUtilsMessengerExt(VkInstance instance,
															  VkDebugUtilsMessengerCreateInfoEXT *debug_info,
															  const VkAllocationCallbacks *allocator,
															  VkDebugUtilsMessengerEXT *messenger);

/* ==================================================
   CONTEXT CORE
   ================================================== */

internal G_Context G_ContextInit(Arena *arena,
								 LOG_Channel log_channel,
								 const G_Requirements *requirements,
								 PFN_vkDebugUtilsMessengerCallbackEXT vk_debug_callback, void *vk_debug_callback_ctx);

internal void G_ContextDestroy(G_Context *context);

#endif // GRAPHICS_CONTEXT_H
