#include "toolbox.h"

#include <glm/vec2.hpp>

#include "core/common.h"
#include "core/platform.h"

#include "math/calc.h"

#include "core.h"

using namespace mgp;

VkSurfaceFormatKHR vk_toolbox::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableSurfaceFormats)
{
	for (auto &availableFormat : availableSurfaceFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			MGP_LOG("Found *desired* swapchain swap surface format & colour space!");
			return availableFormat;
		}
	}

	MGP_LOG("Could not find *desired* swapchain swap surface format & colour space, falling back...");

	return availableSurfaceFormats[0];
}

VkPresentModeKHR vk_toolbox::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes, bool enableVsync)
{
	if (!enableVsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for (cauto &mode : availablePresentModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vk_toolbox::chooseSwapExtent(const Platform *platform, const VkSurfaceCapabilitiesKHR &capabilities)
{
	if (capabilities.currentExtent.width != CalcU::maxValue())
	{
		return capabilities.currentExtent;
	}
	else
	{
		glm::ivec2 wh = platform->getWindowSize();
		VkExtent2D actualExtent = { (uint32_t)wh.x, (uint32_t)wh.y };

		actualExtent.width  = CalcU::clamp(actualExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width );
		actualExtent.height = CalcU::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

uint32_t vk_toolbox::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((filter & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
			return i;
		}
	}

	MGP_ERROR("Failed to find suitable memory type.");

	return 0;
}

VkFormat vk_toolbox::findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (cauto &fmt : candidates)
	{
		VkFormatProperties properties = {};
		vkGetPhysicalDeviceFormatProperties(device, fmt, &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR  && (properties.linearTilingFeatures  & features) == features) ||
			(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features))
		{
			return fmt;
		}
	}

	MGP_ERROR("Failed to find supported format.");

	return VK_FORMAT_MAX_ENUM;
}

VkFormat vk_toolbox::findDepthFormat(VkPhysicalDevice device)
{
	return findSupportedFormat(
		device,
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool vk_toolbox::hasStencilComponent(VkFormat format)
{
	return (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

uint64_t vk_toolbox::calcShaderBufferAlignedSize(const VkPhysicalDeviceProperties2 &properties, uint64_t size)
{
	const VkDeviceSize &minimumSize = properties.properties.limits.minUniformBufferOffsetAlignment;
	return ((size / minimumSize) * minimumSize) + (((size % minimumSize) > 0) ? minimumSize : 0);
}

SwapchainSupportDetails vk_toolbox::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapchainSupportDetails result = {};

	// get capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &result.capabilities);

	// get surface formats
	uint32_t surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);

	if (surfaceFormatCount)
	{
		result.surfaceFormats.resize(surfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, result.surfaceFormats.data());
	}

	// get present modes
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount)
	{
		result.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, result.presentModes.data());
	}

	return result;
}

bool vk_toolbox::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	if (!extensionCount) {
		MGP_ERROR("Failed to find any device extension properties!");
	}

	std::vector<VkExtensionProperties> availableExts(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExts.data());

	for (cauto &availableExtension : availableExts)
	{
		for (cauto &required : DEVICE_EXTENSIONS)
		{
			if (!cstr::compare(availableExtension.extensionName, required))
				return false;
		}
	}

	return true;
}

uint32_t vk_toolbox::assignPhysicalDeviceUsability(
	VkSurfaceKHR surface,
	VkPhysicalDevice physicalDevice,
	VkPhysicalDeviceProperties2 properties,
	VkPhysicalDeviceFeatures2 features,
	bool *hasEssentials
)
{
	uint32_t resultUsability = 0;

	bool adequateSwapChain = false;
	bool hasRequiredExtensions = checkDeviceExtensionSupport(physicalDevice);

	bool hasAnisotropy = features.features.samplerAnisotropy;

	// prefer / give more weight to discrete gpus than integrated gpus
	if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		resultUsability += 4;
	}
	else if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		resultUsability += 1;
	}

	// if we have anisotropy then that's good i guess lmao
	if (hasAnisotropy)
		resultUsability += 1;

	// it must have the required extensions
	if (hasRequiredExtensions)
	{
		SwapchainSupportDetails details = querySwapChainSupport(physicalDevice, surface);

		adequateSwapChain = (details.surfaceFormats.size() > 0) && (details.presentModes.size() > 0);
		resultUsability += 1;
	}

	// essential features must be satisfied
	if (hasEssentials) {
		(*hasEssentials) = hasRequiredExtensions && adequateSwapChain && hasAnisotropy;
	}

	return resultUsability;
}

VkSampleCountFlagBits vk_toolbox::getMaxUsableSampleCount(const VkPhysicalDeviceProperties2 &properties)
{
	VkSampleCountFlags counts =
		properties.properties.limits.framebufferColorSampleCounts &
		properties.properties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT)
	{
		return VK_SAMPLE_COUNT_64_BIT;
	}
	else if (counts & VK_SAMPLE_COUNT_32_BIT)
	{
		return VK_SAMPLE_COUNT_32_BIT;
	}
	else if (counts & VK_SAMPLE_COUNT_16_BIT)
	{
		return VK_SAMPLE_COUNT_16_BIT;
	}
	else if (counts & VK_SAMPLE_COUNT_8_BIT)
	{
		return VK_SAMPLE_COUNT_8_BIT;
	}
	else if (counts & VK_SAMPLE_COUNT_4_BIT)
	{
		return VK_SAMPLE_COUNT_4_BIT;
	}
	else if (counts & VK_SAMPLE_COUNT_2_BIT)
	{
		return VK_SAMPLE_COUNT_2_BIT;
	}
	else if (counts & VK_SAMPLE_COUNT_1_BIT)
	{
		return VK_SAMPLE_COUNT_1_BIT;
	}

	MGP_ERROR("Could not find a maximum usable sample count!");

	return VK_SAMPLE_COUNT_1_BIT;
}
