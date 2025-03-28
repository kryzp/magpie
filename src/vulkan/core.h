#ifndef VK_CORE_H_
#define VK_CORE_H_

#include "third_party/volk.h"
#include "third_party/vk_mem_alloc.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "core/common.h"
#include "core/app.h"

#include "pipeline_cache.h"
#include "queue.h"
#include "swapchain.h"
#include "descriptor_allocator.h"
#include "descriptor_layout_cache.h"

namespace llt
{
	struct PhysicalDeviceData
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties2 properties;
		VkPhysicalDeviceFeatures2 features;
	};

	class VulkanCore
	{
	public:
		VulkanCore(const Config &config);
		~VulkanCore();

		Swapchain *createSwapchain();

		void swapBuffers();

		void onWindowResize(int width, int height);

		void syncStall() const;

		int getCurrentFrameIdx() const;

		VkCommandPool getGraphicsCommandPool();
		VkCommandPool getTransferCommandPool(int idx = 0);
		VkCommandPool getComputeCommandPool(int idx = 0);

		PipelineCache &getPipelineCache();
		const PipelineCache &getPipelineCache() const;

		DescriptorLayoutCache &getDescriptorLayoutCache();
		const DescriptorLayoutCache &getDescriptorLayoutCache() const;

        VkInstance m_instance;
		VkDevice m_device;
		PhysicalDeviceData m_physicalData;
		VkSampleCountFlagBits m_maxMsaaSamples;
		VmaAllocator m_vmaAllocator;

		Queue m_graphicsQueue;
		Vector<Queue> m_computeQueues;
		Vector<Queue> m_transferQueues;

		Swapchain *m_swapchain;
		VkFormat m_swapChainImageFormat;

		void createImGuiResources();
		ImGui_ImplVulkan_InitInfo getImGuiInitInfo() const;

	private:
		void enumeratePhysicalDevices();
		
		void createLogicalDevice();
		void createCommandPools();
		void createCommandBuffers();
		void createPipelineProcessCache();
		void createVmaAllocator();

		VkSampleCountFlagBits getMaxUsableSampleCount() const;
		void findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		PipelineCache m_pipelineCache;
		DescriptorLayoutCache m_descriptorLayoutCache;

		uint64_t m_currentFrameIdx;
		VkPipelineCache m_pipelineProcessCache;

#if LLT_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif // LLT_DEBUG

		DescriptorPoolStatic m_imGuiDescriptorPool;
	};

	extern VulkanCore *g_vkCore;
}

#endif // VK_CORE_H_
