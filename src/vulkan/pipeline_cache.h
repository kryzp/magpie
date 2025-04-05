#pragma once

#include <array>
#include <unordered_map>

#include "third_party/volk.h"

namespace mgp
{
	class Shader;
	class VertexFormat;
	class BlendState;
	class RenderInfo;
	class VulkanCore;

	class GraphicsPipelineDefinition
	{
	public:
		GraphicsPipelineDefinition();
		~GraphicsPipelineDefinition();

		void setShader(const Shader *shader);
		const Shader *getShader() const;

		void setVertexFormat(const VertexFormat *format);
		const VertexFormat *getVertexFormat() const;

		void setSampleShading(bool enabled, float minSampleShading);

		bool isSampleShadingEnabled() const;
		float getMinSampleShading() const;

		void setCullMode(VkCullModeFlagBits cull);
		VkCullModeFlagBits getCullMode() const;

		void setFrontFace(VkFrontFace front);
		VkFrontFace getFrontFace() const;

		const std::vector<VkPipelineShaderStageCreateInfo> &getShaderStages() const;

		void setDepthOp(VkCompareOp op);
		void setDepthTest(bool enabled);
		void setDepthWrite(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

		const VkPipelineDepthStencilStateCreateInfo &getDepthStencilState() const;

		void setBlendState(const BlendState &state);

		const VkPipelineColorBlendAttachmentState &getColourBlendState() const;

		bool isBlendStateLogicOpEnabled() const;
		VkLogicOp getBlendStateLogicOp() const;

		const std::array<float, 4> &getBlendConstants() const;
		float getBlendConstant(int idx) const;

	private:
		const Shader *m_shader;

		const VertexFormat *m_vertexFormat;

		VkCullModeFlagBits m_cullMode;
		VkFrontFace m_frontFace;

		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

		VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo;

		std::array<float, 4> m_blendConstants;
		VkPipelineColorBlendAttachmentState m_colourBlendState;

		bool m_blendStateLogicOpEnabled;
		VkLogicOp m_blendStateLogicOp;

		bool m_sampleShadingEnabled;
		float m_minSampleShading;
	};

	class ComputePipelineDefinition
	{
	public:
		ComputePipelineDefinition();

		void setShader(const Shader *shader);
		const Shader *getShader() const;

		const VkPipelineShaderStageCreateInfo &getStage() const;

	private:
		const Shader *m_shader;
		VkPipelineShaderStageCreateInfo m_stage;
	};

	struct PipelineData
	{
		VkPipelineBindPoint bindPoint;
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	static constexpr VkDynamicState PIPELINE_DYNAMIC_STATES[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_BLEND_CONSTANTS // todo: add dynamic blend constants
	};

	class PipelineCache
	{
	public:
		PipelineCache();
		~PipelineCache();

		void init(const VulkanCore *core);
		void dispose();

		PipelineData fetchGraphicsPipeline(const GraphicsPipelineDefinition &definition, const RenderInfo &renderInfo);
		PipelineData fetchComputePipeline(const ComputePipelineDefinition &definition);

		VkPipelineLayout fetchPipelineLayout(const Shader *shader);

	private:
		std::unordered_map<uint64_t, VkPipeline> m_pipelines;
		std::unordered_map<uint64_t, VkPipelineLayout> m_layouts;

		const VulkanCore *m_core;
	};
}
