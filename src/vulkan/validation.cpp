#include "validation.h"

#include <vector>

#include "core/common.h"

using namespace mgp;

static bool g_hasValidationLayers = false;

bool checkForValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (int i = 0; i < MGP_ARRAY_LENGTH(vk_validation::VALIDATION_LAYERS); i++)
	{
		bool hasLayer = false;
		const char *layerName0 = vk_validation::VALIDATION_LAYERS[i];

		for (int j = 0; j < availableLayers.size(); j++)
		{
			const char *layerName1 = availableLayers[j].layerName;

			if (cstr::compare(layerName0, layerName1) == 0)
			{
				hasLayer = true;
				break;
			}
		}

		if (!hasLayer)
			return false;
	}

	return true;
}

void vk_validation::trySetValidationLayers(VkInstanceCreateInfo &createInfo, VkDebugUtilsMessengerCreateInfoEXT *debugInfo)
{
#if MGP_DEBUG
	g_hasValidationLayers = checkForValidationLayerSupport();

	if (g_hasValidationLayers)
	{
		MGP_LOG("Validation layer support verified.");

		createInfo.enabledLayerCount = MGP_ARRAY_LENGTH(VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
		createInfo.pNext = debugInfo;
	}
	else
	{
		MGP_LOG("No validation layer support.");

		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;
	}
#endif // MGP_DEBUG
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_validation::vkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData
)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		MGP_ERROR("Validation Layer (SEVERITY: %d) (TYPE: %d): %s", messageSeverity, messageType, pCallbackData->pMessage);

	return VK_FALSE;
}

bool vk_validation::hasValidationLayers()
{
	return g_hasValidationLayers;
}

VkResult vk_validation::createDebugUtilsMessengerExt(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
	const VkAllocationCallbacks *allocator,
	VkDebugUtilsMessengerEXT *debugMessenger
)
{
	auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn)
		return fn(instance, createInfo, allocator, debugMessenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vk_validation::destroyDebugUtilsMessengerExt(
	VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks *allocator
)
{
	auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (fn)
		fn(instance, messenger, allocator);
}
