#pragma once

#include "third_party/volk.h"
#include "third_party/vk_mem_alloc.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "bindless.h"
#include "descriptor.h"
#include "pipeline_cache.h"
#include "surface.h"
#include "queue.h"
#include "render_graph.h"

namespace mgp
{
	struct Config;
	class Platform;
	class Swapchain;

	class VulkanCore
	{
	public:
		VulkanCore(const Config &config, const Platform *platform);
		~VulkanCore();

		Swapchain *createSwapchain();

		void deviceWaitIdle() const;
		void waitForFence(const VkFence &fence) const;
		void resetFence(const VkFence &fence) const;
		
		const VkInstance &getInstance() const;
		const VkDevice &getLogicalDevice() const;

		const VkPhysicalDevice &getPhysicalDevice() const;
		const VkPhysicalDeviceProperties2 &getPhysicalDeviceProperties() const;
		const VkPhysicalDeviceFeatures2 &getPhysicalDeviceFeatures() const;

		const VkSampleCountFlagBits getMaxMSAASamples() const;

		PipelineCache &getPipelineCache();
		const PipelineCache &getPipelineCache() const;

		DescriptorLayoutCache &getDescriptorLayoutCache();
		const DescriptorLayoutCache &getDescriptorLayoutCache() const;

		const Surface &getSurface() const;

		const VmaAllocator &getVMAAllocator() const;

//		Queue &getPresentQueue();
//		const Queue &getPresentQueue() const;

		Queue &getGraphicsQueue();
		const Queue &getGraphicsQueue() const;
		
//		std::vector<Queue> &getComputeQueues();
//		const std::vector<Queue> &getComputeQueues() const;

//		std::vector<Queue> &getTransferQueues();
//		const std::vector<Queue> &getTransferQueues() const;

		BindlessResources &getBindlessResources();
		const BindlessResources &getBindlessResources() const;

		void nextFrame();
		int getCurrentFrameIndex() const;

		RenderGraph &getRenderGraph();
		const RenderGraph &getRenderGraph() const;

		VkFormat getDepthFormat() const;

		void initImGui();

	private:
		void enumeratePhysicalDevices(VkSurfaceKHR surface);
		void createLogicalDevice();
		void createPipelineProcessCache();
		void createVmaAllocator();

		void createQueues();
		void destroyQueues();

		void populateQueueCreateInfos(std::vector<VkDeviceQueueCreateInfo> &infos, const std::vector<float> &priorities);

		void findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		VkInstance m_instance;
		VkDevice m_device;

		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties2 m_physicalDeviceProperties;
		VkPhysicalDeviceFeatures2 m_physicalDeviceFeatures;

		VkFormat m_depthFormat;

		BindlessResources m_bindlessResources;

		VkSampleCountFlagBits m_maxMsaaSamples;
		
		VmaAllocator m_vmaAllocator;

		uint64_t m_currentFrameIndex;

		VkPipelineCache m_pipelineProcessCache;
		PipelineCache m_pipelineCache;

		DescriptorLayoutCache m_descriptorLayoutCache;

		DescriptorPoolStatic m_imGuiDescriptorPool;
		VkFormat m_imGuiImageFormat;

		Surface m_surface;

//		Queue m_presentQueue;
		Queue m_graphicsQueue;
//		std::vector<Queue> m_computeQueues;
//		std::vector<Queue> m_transferQueues;

		RenderGraph m_renderGraph;

		const Platform *m_platform;

#if MGP_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
	};
}
