#include "validation.h"

#include <vector>

#include "core/common.h"

using namespace mgp;

static bool g_hasValidationLayers = false;
static VkDebugUtilsMessengerCreateInfoEXT g_debugInfo = {};

bool checkForValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (int i = 0; i < mgp_ARRAY_LENGTH(vk_validation::VALIDATION_LAYERS); i++)
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

void vk_validation::trySetValidationLayers(VkInstanceCreateInfo &createInfo)
{
	g_debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	g_debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	g_debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	g_debugInfo.pfnUserCallback = vk_validation::vkDebugCallback;
	g_debugInfo.pUserData = nullptr;

	g_hasValidationLayers = checkForValidationLayerSupport();

	if (g_hasValidationLayers)
	{
		mgp_LOG("Validation layer support verified.");

		createInfo.enabledLayerCount = mgp_ARRAY_LENGTH(VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
		createInfo.pNext = &g_debugInfo;
	}
	else
	{
		mgp_LOG("No validation layer support.");

		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_validation::vkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData
)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		mgp_ERROR("Validation Layer (SEVERITY: %d) (TYPE: %d): %s", messageSeverity, messageType, pCallbackData->pMessage);

	return VK_FALSE;
}

bool vk_validation::hasValidationLayers()
{
	return g_hasValidationLayers;
}

VkResult vk_validation::createDebugUtilsMessengerExt(
	VkInstance instance,
	const VkAllocationCallbacks *allocator,
	VkDebugUtilsMessengerEXT *debugMessenger
)
{
	auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn)
		return fn(instance, &g_debugInfo, allocator, debugMessenger);

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
