#include "util.h"
#include "backend.h"

#include "../system_backend.h"

#include "../math/calc.h"

using namespace llt;

VkSurfaceFormatKHR vkutil::chooseSwapSurfaceFormat(const Vector<VkSurfaceFormatKHR>& availableSurfaceFormats)
{
	for (auto& available_format : availableSurfaceFormats) {
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return available_format;
		}
	}

	return availableSurfaceFormats[0];
}

VkPresentModeKHR vkutil::chooseSwapPresentMode(const Vector<VkPresentModeKHR>& availablePresentModes, bool enableVsync)
{
	if (!enableVsync) {
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	for (const auto& available_present_mode : availablePresentModes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return available_present_mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D vkutil::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != CalcU::maxValue())
	{
		return capabilities.currentExtent;
	}
	else
	{
		glm::ivec2 wh = g_systemBackend->getWindowSize();
		VkExtent2D actual_extent = { (uint32_t)wh.x, (uint32_t)wh.y };

		actual_extent.width  = CalcU::clamp(actual_extent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width );
		actual_extent.height = CalcU::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

uint32_t vkutil::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties = {};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((filter & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
			return i;
		}
	}

	LLT_ERROR("[VULKAN:UTIL|DEBUG] Failed to find suitable memory type.");
	return 0;
}

VkFormat vkutil::findSupportedFormat(VkPhysicalDevice device, const Vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (const auto& fmt : candidates)
	{
		VkFormatProperties properties = {};
		vkGetPhysicalDeviceFormatProperties(device, fmt, &properties);

		if ((tiling == VK_IMAGE_TILING_LINEAR  && (properties.linearTilingFeatures  & features) == features) ||
			(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features))
		{
			return fmt;
		}
	}

	LLT_ERROR("[VULKAN:UTIL|DEBUG] Failed to find supported format.");
	return VK_FORMAT_MAX_ENUM;
}

VkFormat vkutil::findDepthFormat(VkPhysicalDevice device)
{
	return findSupportedFormat(
		device,
		{
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool vkutil::hasStencilComponent(VkFormat format)
{
	return (format == VK_FORMAT_D32_SFLOAT) || (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT);
}

VkCommandBuffer vkutil::beginSingleTimeCommands(VkCommandPool cmdPool, VkDevice device)
{
	// first we allocate the command buffer then we begin recording onto the command buffer
	// then once we are finished we call the sister function end_single_time_commands.

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmd_buf = {};

	if (VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &cmd_buf); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:UTIL|DEBUG] Failed to reallocate command buffers when copying buffer: %d", result);
	}

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (VkResult result = vkBeginCommandBuffer(cmd_buf, &begin_info); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:UTIL|DEBUG] Failed to begin command buffer when copying buffer: %d", result);
	}

	return cmd_buf;
}

void vkutil::endSingleTimeCommands(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkDevice device, VkQueue graphics)
{
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmdBuffer;

	vkQueueSubmit(graphics, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics);

	vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
}

void vkutil::endSingleTimeGraphicsCommands(VkCommandBuffer cmdBuffer)
{
	vkutil::endSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool, cmdBuffer, g_vulkanBackend->device, g_vulkanBackend->graphicsQueue.getQueue());
}

uint64_t vkutil::calcShaderBufferAlignedSize(uint64_t size, VkPhysicalDeviceProperties properties)
{
	const VkDeviceSize& minimumSize = properties.limits.minUniformBufferOffsetAlignment;
	return (size / minimumSize) * minimumSize + ((size % minimumSize) > 0 ? minimumSize : 0);
}

SwapChainSupportDetails vkutil::querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	SwapChainSupportDetails result = {};

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

bool vkutil::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	if (!extensionCount) {
		LLT_ERROR("[VULKAN:UTIL|DEBUG] Failed to find any device extension properties!");
	}

	Vector<VkExtensionProperties> available_exts(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, available_exts.data());

	Vector<const char*> requiredExtensions(DEVICE_EXTENSIONS, LLT_ARRAY_LENGTH(DEVICE_EXTENSIONS));

	for (const auto& availableExtension : available_exts) {
		for (int i = 0; i < requiredExtensions.size(); i++) {
			if (cstr::compare(availableExtension.extensionName, requiredExtensions[i])) {
				requiredExtensions.erase(i);
			}
		}
	}

	return requiredExtensions.empty();
}

uint32_t vkutil::assignPhysicalDeviceUsability(
	VkSurfaceKHR surface,
	VkPhysicalDevice physicalDevice,
	VkPhysicalDeviceProperties properties,
	VkPhysicalDeviceFeatures features,
	bool* hasEssentials
)
{
	uint32_t resultUsability = 0;

	bool adequateSwapChain = false;
	bool hasRequiredExtensions = checkDeviceExtensionSupport(physicalDevice);

	bool hasGraphics = g_vulkanBackend->graphicsQueue.getFamilyIdx().hasValue();
	bool hasAnisotropy = features.samplerAnisotropy;

	// prefer / give more weight to discrete gpus than integrated gpus
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		resultUsability += 4;
	} else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
		resultUsability += 1;
	}

	// if we have anisotropy then thats good
	if (hasAnisotropy) {
		resultUsability += 1;
	}

	// it must have the required extensions
	if (hasRequiredExtensions) {
		SwapChainSupportDetails swap_chain_support_details = querySwapChainSupport(physicalDevice, surface);
		adequateSwapChain = swap_chain_support_details.surfaceFormats.any() && swap_chain_support_details.presentModes.any();
		resultUsability += 1;
	}

	// essential features must be satisfied
	if (hasEssentials) {
		(*hasEssentials) = hasGraphics && hasRequiredExtensions && adequateSwapChain && hasAnisotropy;
	}

	return resultUsability;
}
