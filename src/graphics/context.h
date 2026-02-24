#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/types.h"
#include "container/vector.h"

namespace gfx
{
	constexpr static u32 FRAMES_IN_FLIGHT = 3;

	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		Vector<VkSurfaceFormatKHR> surface_formats;
		Vector<VkPresentModeKHR> present_modes;
	};

	struct HardwareQueue {
		VkQueue handle = VK_NULL_HANDLE;
		u32 family_index = -1u;

		bool is_valid() const
		{
			return handle != VK_NULL_HANDLE;
		}
	};

	class Context {
	public:
		Context();
		~Context();

		void init();
		void destroy();

		VkInstance get_instance() const;
		VkDevice get_device() const;
		
		VkPhysicalDevice get_physical_device() const;
		const VkPhysicalDeviceProperties2 &get_physical_properties() const;

		VkSurfaceKHR get_surface() const;

		VmaAllocator get_allocator() const;
		
		HardwareQueue graphics() const;

		VkFormat get_depth_format() const;
		VkSampleCountFlagBits get_max_msaa_samples() const;

		SwapchainSupportDetails get_swapchain_details() const;

	private:
		VkInstance instance;
		VkDevice device;

		VkPhysicalDevice physical_device;
		VkPhysicalDeviceProperties2 physical_device_properties;
		VkPhysicalDeviceFeatures2 physical_device_features;
		
		VkSurfaceKHR surface;

		VmaAllocator vma_allocator;

		HardwareQueue graphics_queue;

		VkDebugUtilsMessengerEXT debug_messenger;
		bool has_validation_layers;

		VkFormat depth_format;
		VkSampleCountFlagBits max_msaa_samples;

		SwapchainSupportDetails swapchain_details;
	};
}
