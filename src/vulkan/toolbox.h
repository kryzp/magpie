#pragma once

#include <vector>

#include "third_party/volk.h"

namespace mgp
{
	class Platform;
	class VulkanCore;

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		std::vector<VkPresentModeKHR> presentModes;
	};
}

namespace mgp::vk_toolbox
{
	static constexpr char *DEVICE_EXTENSIONS[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
#ifdef LLT_MAC_SUPPORT
		"VK_KHR_portability_subset"
#endif // LLT_MAC_SUPPORT
	};

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableSurfaceFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes, bool enableVsync);
	VkExtent2D chooseSwapExtent(const Platform *platform, const VkSurfaceCapabilitiesKHR &capabilities);

	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t filter, VkMemoryPropertyFlags properties);

	VkFormat findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat(VkPhysicalDevice device);

	bool hasStencilComponent(VkFormat format);

	uint64_t calcShaderBufferAlignedSize(const VkPhysicalDeviceProperties2 &properties, uint64_t size);

	SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	uint32_t assignPhysicalDeviceUsability(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2 properties, VkPhysicalDeviceFeatures2 features, bool *hasEssentials);

	VkPipelineStageFlags2 getTransferPipelineStageFlags(VkImageLayout layout);
	VkAccessFlags2 getTransferAccessFlags(VkImageLayout layout);

	VkSampleCountFlagBits getMaxUsableSampleCount(const VkPhysicalDeviceProperties2 &properties);
}
