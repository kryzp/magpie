#pragma once

#include <inttypes.h>

#include <string>

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang-com-helper.h>

#include "core/config.h"

#include "constants.h"
#include "surface.h"
#include "queue.h"
#include "command_buffer.h"
#include "sampler.h"
#include "descriptor.h"
#include "pipeline.h"

namespace mgp
{
	class Swapchain;
	class DescriptorPoolStatic;
	class Shader;
	class ShaderStage;
	class PlatformCore;

	class GraphicsCore
	{
	public:
		GraphicsCore(const Config &config, PlatformCore *platform);
		~GraphicsCore();
		
		CommandBuffer *beginPresent();
		void present();

		CommandBuffer *beginInstantSubmit();
		void submit(CommandBuffer *cmd);

		void waitIdle();
		
		VkFormat getDepthFormat();

		void initImGui();

		Swapchain *getSwapchain();
		
		Image *createImage(
			uint32_t width, uint32_t height, uint32_t depth,
			VkFormat format,
			VkImageViewType type,
			VkImageTiling tiling,
			uint32_t mipmaps,
			VkSampleCountFlagBits samples,
			bool transient,
			bool storage
		);
		
		ImageView *createImageView(
			Image *image,
			int layerCount,
			int layer,
			int baseMipLevel
		);
		
		Sampler *createSampler(const SamplerStyle &style);
		
		ShaderStage *createShaderStage(
			VkShaderStageFlagBits type,
			const std::string &path
		);

		Shader *createShader(
			uint64_t pushConstantsSize,
			const std::vector<DescriptorLayout *> &layouts,
			const std::vector<ShaderStage *> &stages
		);
		
		GPUBuffer *createGPUBuffer(
			VkBufferUsageFlags usage,
			VmaAllocationCreateFlagBits flags,
			uint64_t size
		);
		
		DescriptorLayout *createDescriptorLayout(
			VkShaderStageFlags shaderStages,
			const std::vector<DescriptorLayoutBinding> &bindings,
			VkDescriptorSetLayoutCreateFlags flags
		);
		
		DescriptorPoolStatic *createStaticDescriptorPool(uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes);
		DescriptorPool *createDescriptorPool(uint32_t initialSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes);

		VkPipelineLayout createPipelineLayout(const Shader *shader);

		VkPipeline createGraphicsPipeline(VkPipelineLayout layout, const GraphicsPipelineDef &definition, const RenderInfo &renderInfo);
		VkPipeline createComputePipeline(VkPipelineLayout layout, const ComputePipelineDef &definition);

	public:
		void waitForFence(const VkFence &fence) const;
		void resetFence(const VkFence &fence) const;

		int getCurrentFrameIndex() const { return m_currentFrameIndex; }

		const VkInstance &getInstance() const { return m_instance; }
		const VkDevice &getLogicalDevice() const { return m_device; }

		const VkPhysicalDevice &getPhysicalDevice() const { return m_physicalDevice; }
		const VkPhysicalDeviceProperties2 &getPhysicalDeviceProperties() const { return m_physicalDeviceProperties; }
		const VkPhysicalDeviceFeatures2 &getPhysicalDeviceFeatures() const { return m_physicalDeviceFeatures; }

		const VkSampleCountFlagBits getMaxMSAASamples() const { return m_maxMsaaSamples; }

		const Surface &getSurface() const { return m_surface; }

		const VmaAllocator &getVMAAllocator() const { return m_vmaAllocator; }

		VkPipelineCache getProcessCache() { return m_pipelineProcessCache; }
		const VkPipelineCache &getProcessCache() const { return m_pipelineProcessCache; }

		Queue& getGraphicsQueue() { return m_graphicsQueue; }
		const Queue& getGraphicsQueue() const { return m_graphicsQueue; }
		
		const Slang::ComPtr<slang::ISession> &getSlangSession() const { return m_slangSession; }

	private:
		void enumeratePhysicalDevices(VkSurfaceKHR surface);
		void createLogicalDevice();
		void createPipelineProcessCache();
		void createVmaAllocator();
		void findQueueFamilies();
		void initSlang();

		PlatformCore *m_platform;

		VkInstance m_instance;
		VkDevice m_device;

		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties2 m_physicalDeviceProperties;
		VkPhysicalDeviceFeatures2 m_physicalDeviceFeatures;

		VkFormat m_depthFormat;

		VkSampleCountFlagBits m_maxMsaaSamples;
		
		VmaAllocator m_vmaAllocator;

		uint64_t m_currentFrameIndex;

		VkPipelineCache m_pipelineProcessCache;

		DescriptorPoolStatic *m_imGuiDescriptorPool;
		VkFormat m_imGuiImageFormat;

		Swapchain *m_swapchain;
		Surface m_surface;
		Queue m_graphicsQueue;

		Slang::ComPtr<slang::IGlobalSession> m_slangGlobalSession;
		Slang::ComPtr<slang::ISession> m_slangSession;

		CommandBuffer m_inFlightCmd;

#if MGP_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
	};
}
