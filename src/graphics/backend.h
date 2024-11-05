#ifndef VK_BACKEND_H_
#define VK_BACKEND_H_

#include <vulkan/vulkan.h>

#include "../container/vector.h"
#include "../container/array.h"
#include "../container/optional.h"
#include "../container/hash_map.h"
#include "../container/bitset.h"

#include "../math/rect.h"

#include "../common.h"
#include "../app.h"

#include "render_pass_builder.h"

#include "descriptor_pool_mgr.h"
#include "descriptor_builder.h"
#include "descriptor_cache.h"

#include "shader_buffer.h"

#include "gpu_buffer.h"
#include "texture.h"
#include "shader.h"
#include "render_target.h"
#include "backbuffer.h"
#include "render_pass.h"
#include "queue.h"

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

		void beginRender();
		void render(const RenderOp& op);
		void endRender();

		void beginCompute();
		void dispatchCompute(int gcX, int gcY, int gcZ);
		void endCompute();

		void syncComputeWithNextRender();

        Backbuffer* createBackbuffer();

		void swapBuffers();

		void setRenderTarget(GenericRenderTarget* target);

		void onWindowResize(int width, int height);

		void toggleBlending(bool enabled);
		void setBlendWriteMask(bool r, bool g, bool b, bool a);
		void setBlendColour(const Blend& blend);
		void setBlendAlpha(const Blend& blend);
		void setBlendConstants(float r, float g, float b, float a);
		void getBlendConstants(float* constants);
		void setBlendOp(bool enabled, VkLogicOp op);

		void setDepthParams(bool depthTest, bool depthWrite);
		void setDepthOp(VkCompareOp op);
		void setDepthTest(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

		void setViewport(const RectF& rect);
		void setScissor(const RectI& rect);

		void setSampleShading(bool enabled, float minSampleShading);
		void setCullMode(VkCullModeFlagBits cull);

		void setTexture(uint32_t idx, const Texture* texture);
		void setSampler(uint32_t idx, TextureSampler* sampler);

		void bindShader(const ShaderProgram* shader);

		void pushShaderParams(int bufferIdx, int bindIdx, VkShaderStageFlagBits type, ShaderParameters& params);
		void pushShaderBuffer(int bufferIdx, int bindIdx, VkShaderStageFlagBits type, void* data, uint64_t size);

		void bindShaderParams(int bufferIdx, int bindIdx);
		void bindShaderBuffer(int bufferIdx, int bindIdx);

		void unbindShaderParams(int bufferIdx);
		void unbindShaderBuffer(int bufferIdx);

		void setPushConstants(ShaderParameters& params);
		void resetPushConstants();

		void syncStall() const;

		void clearDescriptorCacheAndPool();

		int getCurrentFrameIdx() const;

        VkInstance vulkanInstance;
		VkDevice device;
		PhysicalDeviceData physicalData;
		VkSampleCountFlagBits msaaSamples;
		VkFormat swapChainImageFormat;

		Queue graphicsQueue;
		Vector<Queue> computeQueues;
		Vector<Queue> transferQueues;

	private:
		void enumeratePhysicalDevices();
		
		void createLogicalDevice();
		void createCommandPools();
		void createCommandBuffers();
		void createPipelineProcessCache();
		void createComputeResources();

		Vector<uint32_t> getDynamicOffsets() const;

		VkSampleCountFlagBits getMaxUsableSampleCount() const;
		void findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		void clearPipelineCache();

		VkPipelineLayout getPipelineLayout(VkShaderStageFlagBits stage);
		VkPipeline getGraphicsPipeline();
		VkPipeline getComputePipeline();

		void resetDescriptorBuilder();

		DescriptorBuilder getDescriptorBuilder(VkShaderStageFlagBits stage);
		VkDescriptorSet getDescriptorSet(VkShaderStageFlagBits stage);

		// pipelines
		VkPipelineCache m_pipelineProcessCache;
		HashMap<uint64_t, VkPipeline> m_graphicsPipelineCache;
		HashMap<uint64_t, VkPipelineLayout> m_pipelineLayoutCache;
		HashMap<uint64_t, VkPipeline> m_computePipelineCache;
		VkPipelineShaderStageCreateInfo m_computeShaderStageInfo;
		Array<VkSemaphore, mgc::FRAMES_IN_FLIGHT> m_computeFinishedSemaphores;
		bool m_uncertainComputeFinished;

		// render pass
		RenderPassBuilder* m_currentRenderPassBuilder;
		Array<VkDescriptorImageInfo, mgc::MAX_BOUND_TEXTURES> m_imageInfos;
		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_graphicsShaderStages;
		GenericRenderTarget* m_currentRenderTarget;

		// descriptors
		DescriptorPoolMgr m_descriptorPoolManager;
		DescriptorCache m_descriptorCache;
		DescriptorBuilder m_descriptorBuilder;
		bool m_descriptorBuilderDirty;

		// swap chain
        Backbuffer* m_backbuffer;
        VkViewport m_viewport;
		VkRect2D m_scissor;
		uint64_t m_currentFrameIdx;

		// shader parameters
		ShaderParameters::PackedData m_pushConstants;

		Array<ShaderBuffer, mgc::MAX_BOUND_UBOS> m_uboManagers;
		Array<ShaderBuffer, mgc::MAX_BOUND_SSBOS> m_ssboManagers;

		// rendering configs
		VkPipelineDepthStencilStateCreateInfo m_depthStencilCreateInfo;
		Array<VkPipelineColorBlendAttachmentState, mgc::MAX_RENDER_TARGET_ATTACHMENTS> m_colourBlendAttachmentStates;
		float m_blendConstants[4];
		bool m_sampleShadingEnabled;
		float m_minSampleShading;
		bool m_blendStateLogicOpEnabled;
		VkLogicOp m_blendStateLogicOp;
		VkCullModeFlagBits m_cullMode;

#if LLT_DEBUG
		VkDebugUtilsMessengerEXT m_debugMessenger;
#endif // LLT_DEBUG
	};

	extern VulkanBackend* g_vulkanBackend;
}

#endif // VK_BACKEND_H_
