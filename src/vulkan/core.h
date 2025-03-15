#ifndef VK_CORE_H_
#define VK_CORE_H_

#define VK_NO_PROTOTYPES
#include "third_party/volk.h"

#include "third_party/vk_mem_alloc.h"

#include "third_party/imgui/imgui_impl_vulkan.h"

#include "container/vector.h"
#include "container/array.h"
#include "container/optional.h"
#include "container/hash_map.h"
#include "container/bitset.h"

#include "math/rect.h"

#include "core/common.h"
#include "core/app.h"

#include "render_info.h"

#include "descriptor_allocator.h"
#include "descriptor_builder.h"
#include "descriptor_layout_cache.h"

#include "vertex_format.h"

#include "shader_buffer.h"

#include "gpu_buffer.h"
#include "texture.h"
#include "shader.h"
#include "render_target.h"
#include "swapchain.h"
#include "queue.h"
#include "pipeline.h"

#include "rendering/shader_buffer_mgr.h"
#include "rendering/gpu_buffer_mgr.h"
#include "rendering/texture_mgr.h"
#include "rendering/shader_mgr.h"
#include "rendering/render_target_mgr.h"

namespace llt
{
	struct PhysicalDeviceData
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
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
		VkFormat m_swapChainImageFormat;
		VmaAllocator m_vmaAllocator;

		Queue m_graphicsQueue;
		Vector<Queue> m_computeQueues;
		Vector<Queue> m_transferQueues;

		Swapchain *m_swapchain;

		void createImGuiResources();
		ImGui_ImplVulkan_InitInfo getImGuiInitInfo() const;
		const VkFormat &getImGuiAttachmentFormat() const;

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
