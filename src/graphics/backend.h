#ifndef VK_BACKEND_H_
#define VK_BACKEND_H_

#include <vulkan/vulkan.h>

#include "../container/vector.h"
#include "../container/array.h"
#include "../container/optional.h"
#include "../container/hash_map.h"

#include "../math/rect.h"

#include "../common.h"
#include "../app.h"

#include "render_pass_builder.h"

#include "descriptor_pool_mgr.h"
#include "descriptor_builder.h"
#include "descriptor_cache.h"

#include "ubo_manager.h"

#include "buffer.h"
#include "texture.h"
#include "shader.h"
#include "render_target.h"
#include "backbuffer.h"
#include "render_pass.h"
#include "queue.h"

#include "buffer_mgr.h"
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

		void setDepthParams(bool depth_test, bool depthWrite);
		void setDepthOp(VkCompareOp op);
		void setDepthBoundsTest(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

		void setViewport(const RectF& rect);
		void setScissor(const RectI& rect);

		void setSampleShading(bool enabled, float minSampleShading);
		void setCullMode(VkCullModeFlagBits cull);

		void setTexture(uint32_t idx, const Texture* texture);
		void setSampler(uint32_t idx, TextureSampler* sampler);

		void bindShader(const ShaderProgram* shader);
		void bindShaderParams(VkShaderStageFlagBits shader_type, ShaderParameters& params);

		void setPushConstants(ShaderParameters& params);
		void resetPushConstants();

		void syncStall() const;

		void clearDescriptorSetAndPool();

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
		void createCommandPools(uint32_t graphicsFamilyIdx);
		void createCommandBuffers();
		void createPipelineProcessCache();
		VkSampleCountFlagBits getMaxUsableSampleCount() const;
		void findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		void clearPipelineCache();

		VkPipeline getGraphicsPipeline();
		VkPipelineLayout getGraphicsPipelineLayout();

		void resetDescriptorBuilder();

		DescriptorBuilder getDescriptorBuilder();
		VkDescriptorSet getDescriptorSet();

		// pipeline
		HashMap<uint64_t, VkPipeline> m_pipelineCache;
		HashMap<uint64_t, VkPipelineLayout> m_pipelineLayoutCache;
		VkPipelineCache m_pipelineProcessCache;

		// render pass
		RenderPassBuilder* m_currentRenderPassBuilder;
		Array<VkDescriptorImageInfo, mgc::MAX_BOUND_TEXTURES> m_imageInfos;
		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_shaderStages;
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

		// shader parameters
		UBOManager m_uboManager;
		//Array<const ShaderParameters*, SHADER_TYPE_GRAPHICS_COUNT> m_current_shader_parameters;
		ShaderParameters::PackedData m_pushConstants;

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
