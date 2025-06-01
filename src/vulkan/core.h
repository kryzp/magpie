#pragma once

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang-com-helper.h>

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

		void nextFrame();
		int getCurrentFrameIndex() const;

		void initImGui();
		
	public:
		const VkInstance &getInstance() const { return m_instance; }
		const VkDevice &getLogicalDevice() const { return m_device; }

		const VkPhysicalDevice &getPhysicalDevice() const { return m_physicalDevice; }
		const VkPhysicalDeviceProperties2 &getPhysicalDeviceProperties() const { return m_physicalDeviceProperties; }
		const VkPhysicalDeviceFeatures2 &getPhysicalDeviceFeatures() const { return m_physicalDeviceFeatures; }

		const VkSampleCountFlagBits getMaxMSAASamples() const { return m_maxMsaaSamples; }

		PipelineCache &getPipelineCache() { return m_pipelineCache; }
		const PipelineCache &getPipelineCache() const { return m_pipelineCache; }

		DescriptorLayoutCache &getDescriptorLayoutCache() { return m_descriptorLayoutCache; }
		const DescriptorLayoutCache &getDescriptorLayoutCache() const { return m_descriptorLayoutCache; }

		const Surface &getSurface() const { return m_surface; }

		const VmaAllocator &getVMAAllocator() const { return m_vmaAllocator; }

		BindlessResources &getBindlessResources() { return m_bindlessResources; }
		const BindlessResources &getBindlessResources() const { return m_bindlessResources; }

		VkPipelineCache getProcessCache() { return m_pipelineProcessCache; }
		const VkPipelineCache &getProcessCache() const { return m_pipelineProcessCache; }

		Queue& getGraphicsQueue() { return m_graphicsQueue; }
		const Queue& getGraphicsQueue() const { return m_graphicsQueue; }

		//std::vector<Queue> &getComputeQueues();
		//const std::vector<Queue> &getComputeQueues() const;

		//std::vector<Queue> &getTransferQueues();
		//const std::vector<Queue> &getTransferQueues() const;

		RenderGraph &getRenderGraph() { return m_renderGraph; }
		const RenderGraph &getRenderGraph() const { return m_renderGraph; }

		VkFormat getDepthFormat() const { return m_depthFormat; }

		const Slang::ComPtr<slang::ISession> &getSlangSession() const { return m_slangSession; }

	private:
		void enumeratePhysicalDevices(VkSurfaceKHR surface);
		void createLogicalDevice();
		void createPipelineProcessCache();
		void createVmaAllocator();
		void findQueueFamilies();
		void initSlang();

		VkInstance m_instance;
		VkDevice m_device;

		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties2 m_physicalDeviceProperties;
		VkPhysicalDeviceFeatures2 m_physicalDeviceFeatures;

		const Platform *m_platform;

		VkFormat m_depthFormat;

		VkSampleCountFlagBits m_maxMsaaSamples;
		
		VmaAllocator m_vmaAllocator;

		uint64_t m_currentFrameIndex;

		VkPipelineCache m_pipelineProcessCache;
		PipelineCache m_pipelineCache;

		DescriptorLayoutCache m_descriptorLayoutCache;

		DescriptorPoolStatic m_imGuiDescriptorPool;
		VkFormat m_imGuiImageFormat;

		Surface m_surface;

		Queue m_graphicsQueue;
//		std::vector<Queue> m_computeQueues;
//		std::vector<Queue> m_transferQueues;

		RenderGraph m_renderGraph;
		BindlessResources m_bindlessResources;

		Slang::ComPtr<slang::IGlobalSession> m_slangGlobalSession;
		Slang::ComPtr<slang::ISession> m_slangSession;

#if MGP_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
	};
}
