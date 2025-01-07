#ifndef VK_BACKEND_H_
#define VK_BACKEND_H_

#include <vulkan/vulkan.h>

#include "../third_party/vk_mem_alloc.h"

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

#include "vertex_descriptor.h"

#include "shader_buffer.h"

#include "gpu_buffer.h"
#include "texture.h"
#include "shader.h"
#include "render_target.h"
#include "backbuffer.h"
#include "render_pass.h"
#include "queue.h"

#include "graphics_pipeline.h"
#include "compute_pipeline.h"

#include "shader_buffer_mgr.h"
#include "gpu_buffer_mgr.h"
#include "texture_mgr.h"
#include "shader_mgr.h"
#include "render_target_mgr.h"

namespace llt
{
	enum CullMode
	{
		CULL_MODE_NONE = 0,
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
		VulkanBackend(const Config& config);
		~VulkanBackend();

        Backbuffer* createBackbuffer();

		void swapBuffers();

		void onWindowResize(int width, int height);

		void setPushConstants(ShaderParameters& params);
		void resetPushConstants();

		void waitOnCompute();
		void syncStall() const;

		int getCurrentFrameIdx() const;

		GenericRenderTarget* getRenderTarget();
		const GenericRenderTarget* getRenderTarget() const;

		void beginGraphics(GenericRenderTarget* target = nullptr);
		void endGraphics();

		void beginCompute();
		void endCompute();

        VkInstance instance;
		VkDevice device;
		PhysicalDeviceData physicalData;
		VkSampleCountFlagBits maxMsaaSamples;
		VkFormat swapChainImageFormat;
		VmaAllocator vmaAllocator;

		HashMap<uint64_t, VkPipeline> pipelineCache;
		HashMap<uint64_t, VkPipelineLayout> pipelineLayoutCache;

		Queue graphicsQueue;
		Vector<Queue> computeQueues;
		Vector<Queue> transferQueues;

		ShaderParameters::PackedData pushConstants;

	private:
		void enumeratePhysicalDevices();
		
		void createLogicalDevice();
		void createCommandPools();
		void createCommandBuffers();
		void createPipelineProcessCache();
		void createComputeResources();
		void createVmaAllocator();

		VkSampleCountFlagBits getMaxUsableSampleCount() const;
		void findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		void clearPipelineCache();

		Array<VkSemaphore, mgc::FRAMES_IN_FLIGHT> m_computeFinishedSemaphores;

		Backbuffer* m_backbuffer;
		uint64_t m_currentFrameIdx;
		GenericRenderTarget* m_currentRenderTarget;
		VkPipelineCache m_pipelineProcessCache;
		bool m_waitOnCompute;

#if LLT_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif // LLT_DEBUG
	};

	extern VulkanBackend* g_vulkanBackend;
}

#endif // VK_BACKEND_H_
