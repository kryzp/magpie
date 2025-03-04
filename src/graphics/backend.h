#ifndef VK_BACKEND_H_
#define VK_BACKEND_H_

#define VK_NO_PROTOTYPES
#include "../third_party/volk.h"

#include "../third_party/vk_mem_alloc.h"

#include "../third_party/imgui/imgui_impl_vulkan.h"

#include "../container/vector.h"
#include "../container/array.h"
#include "../container/optional.h"
#include "../container/hash_map.h"
#include "../container/bitset.h"

#include "../math/rect.h"

#include "../common.h"
#include "../app.h"

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
#include "backbuffer.h"
#include "render_pass.h"
#include "queue.h"
#include "pipeline.h"

#include "shader_buffer_mgr.h"
#include "gpu_buffer_mgr.h"
#include "texture_mgr.h"
#include "shader_mgr.h"
#include "render_target_mgr.h"

namespace llt
{
	enum CullMode
	{
		CULL_MODE_BACK,
		CULL_MODE_FRONT,
		CULL_MODE_FRONT_AND_BACK,
		CULL_MODE_MAX_ENUM
	};

	struct PhysicalDeviceData
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
	};

	class VulkanBackend
	{
	public:
		VulkanBackend(const Config &config);
		~VulkanBackend();

        Backbuffer *createBackbuffer();

		void swapBuffers();

		void onWindowResize(int width, int height);

		void setPushConstants(ShaderParameters &params);
		void resetPushConstants();

		void syncStall() const;

		int getCurrentFrameIdx() const;

		VkCommandPool getGraphicsCommandPool();
		VkCommandPool getTransferCommandPool(int idx = 0);
		VkCommandPool getComputeCommandPool(int idx = 0);

        VkInstance m_instance;
		VkDevice m_device;
		PhysicalDeviceData m_physicalData;
		VkSampleCountFlagBits m_maxMsaaSamples;
		VkFormat m_swapChainImageFormat;
		VmaAllocator m_vmaAllocator;

		HashMap<uint64_t, VkPipeline> m_pipelineCache;
		HashMap<uint64_t, VkPipelineLayout> m_pipelineLayoutCache;

		Queue m_graphicsQueue;
		Vector<Queue> m_computeQueues;
		Vector<Queue> m_transferQueues;

		ShaderParameters::PackedData m_pushConstants;

		Backbuffer *m_backbuffer;

		void createImGuiResources();
		ImGui_ImplVulkan_InitInfo getImGuiInitInfo() const;
		VkFormat getImGuiAttachmentFormat() const;

	private:
		void enumeratePhysicalDevices();
		
		void createLogicalDevice();
		void createCommandPools();
		void createCommandBuffers();
		void createPipelineProcessCache();
		void createVmaAllocator();

		VkSampleCountFlagBits getMaxUsableSampleCount() const;
		void findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		void clearPipelineCache();

		uint64_t m_currentFrameIdx;
		VkPipelineCache m_pipelineProcessCache;

#if LLT_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif // LLT_DEBUG

		DescriptorPoolStatic m_imGuiDescriptorPool;
	};

	extern VulkanBackend *g_vulkanBackend;
}

#endif // VK_BACKEND_H_
