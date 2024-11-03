#ifndef VK_UTIL_H_
#define VK_UTIL_H_

#include <vulkan/vulkan.h>

#include "../common.h"

#include "../container/vector.h"
#include "../container/optional.h"
#include "../container/array.h"

#include "blend.h"
#include "shader.h"
#include "texture.h"
#include "texture_sampler.h"

namespace llt
{
	class VulkanBackend;

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		Vector<VkSurfaceFormatKHR> surfaceFormats;
		Vector<VkPresentModeKHR> presentModes;
	};

	namespace vkutil
	{
		/*
		 * The list of required validation layer names we need.
		 */
		static const char* VALIDATION_LAYERS[] = {
			"VK_LAYER_KHRONOS_validation"
		};

		/*
		 * The list of device extensions that we need.
		 */
		static const char* DEVICE_EXTENSIONS[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			//"VK_KHR_portability_subset"
		};

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const Vector<VkSurfaceFormatKHR>& availableSurfaceFormats);
		VkPresentModeKHR chooseSwapPresentMode(const Vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t filter, VkMemoryPropertyFlags properties);

		VkFormat findSupportedFormat(VkPhysicalDevice device, const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat findDepthFormat(VkPhysicalDevice device);

		bool hasStencilComponent(VkFormat format);

		VkCommandBuffer beginSingleTimeCommands(VkCommandPool cmdPool, VkDevice device);
		void endSingleTimeCommands(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkDevice device, VkQueue graphics);
		void endSingleTimeGraphicsCommands(VkCommandBuffer cmdBuffer);

		uint64_t calcUboAlignedSize(uint64_t size, VkPhysicalDeviceProperties properties);

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
		uint32_t assignPhysicalDeviceUsability(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties properties, VkPhysicalDeviceFeatures features, bool* hasEssentials);
	}
}

#endif // VK_UTIL_H_
