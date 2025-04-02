#pragma once

#include "third_party/volk.h"

namespace mgp::vk_validation
{
	static const char *VALIDATION_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
	};

	void trySetValidationLayers(
		VkInstanceCreateInfo &createInfo,
		VkDebugUtilsMessengerCreateInfoEXT *debugInfo
	);

	bool hasValidationLayers();

	VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData
	);

	VkResult createDebugUtilsMessengerExt(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
		const VkAllocationCallbacks *allocator,
		VkDebugUtilsMessengerEXT *debugMessenger
	);

	void destroyDebugUtilsMessengerExt(
		VkInstance instance,
		VkDebugUtilsMessengerEXT messenger,
		const VkAllocationCallbacks *allocator
	);
}
