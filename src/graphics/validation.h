#pragma once

#include <Volk/volk.h>

namespace mgp::vk_validation
{
	static const char *VALIDATION_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
	};

	bool hasValidationLayers();
	void trySetValidationLayers(VkInstanceCreateInfo &createInfo);

	VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData
	);

	VkResult createDebugUtilsMessengerExt(
		VkInstance instance,
		const VkAllocationCallbacks *allocator,
		VkDebugUtilsMessengerEXT *debugMessenger
	);

	void destroyDebugUtilsMessengerExt(
		VkInstance instance,
		VkDebugUtilsMessengerEXT messenger,
		const VkAllocationCallbacks *allocator
	);
}
