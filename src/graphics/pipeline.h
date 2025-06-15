#pragma once

#include <array>
#include <unordered_map>

#include <Volk/volk.h>

#include "blend.h"

namespace mgp
{
	class VertexFormat;
	class RenderInfo;

	class Shader;
	class GraphicsCore;

	class GraphicsPipelineDef
	{
	public:
		GraphicsPipelineDef();
		~GraphicsPipelineDef() = default;

		void setShader(const Shader *shader);
		const Shader *getShader() const;

		void setVertexFormat(const VertexFormat *format);
		const VertexFormat *getVertexFormat() const;

		void setSampleShading(bool enabled, float minSampleShading);

		bool isSampleShadingEnabled() const;
		float getMinSampleShading() const;

		void setCullMode(VkCullModeFlags cull);
		VkCullModeFlags getCullMode() const;

		void setFrontFace(VkFrontFace front);
		VkFrontFace getFrontFace() const;

		void setDepthOp(VkCompareOp op);
		void setDepthTest(bool enabled);
		void setDepthWrite(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

		void setBlendState(const BlendState &state);

		const VkPipelineColorBlendAttachmentState &getColourBlendState() const;
		const VkPipelineDepthStencilStateCreateInfo &getDepthStencilState() const;

		bool isBlendStateLogicOpEnabled() const;
		VkLogicOp getBlendStateLogicOp() const;

		const std::array<float, 4> &getBlendConstants() const;
		float getBlendConstant(int idx) const;

		uint64_t getHash() const;

	private:
		const Shader *m_shader;

		const VertexFormat *m_vertexFormat;

		VkCullModeFlags m_cullMode;
		VkFrontFace m_frontFace;

		std::array<float, 4> m_blendConstants;

		VkPipelineColorBlendAttachmentState m_colourBlendState;
		VkPipelineDepthStencilStateCreateInfo m_depthStencilState;

		bool m_blendStateLogicOpEnabled;
		VkLogicOp m_blendStateLogicOp;

		bool m_sampleShadingEnabled;
		float m_minSampleShading;
	};

	class ComputePipelineDef
	{
	public:
		ComputePipelineDef();

		void setShader(const Shader *shader);
		const Shader *getShader() const;
		
		uint64_t getHash() const;

	private:
		const Shader *m_shader;
	};

	static constexpr VkDynamicState PIPELINE_DYNAMIC_STATES[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_BLEND_CONSTANTS // todo: add dynamic blend constants
	};

	struct PipelineState
	{
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	class PipelineCache
	{
	public:
		PipelineCache() = default;
		~PipelineCache() = default;

		void init(GraphicsCore *gfx);
		void destroy();

		PipelineState fetchGraphicsPipeline(const GraphicsPipelineDef &definition, const RenderInfo &renderInfo);
		PipelineState fetchComputePipeline(const ComputePipelineDef &definition);

	private:
		GraphicsCore *m_gfx;
		
		VkPipelineLayout fetchPipelineLayout(const Shader *shader);

		std::unordered_map<uint64_t, VkPipeline> m_pipelines;
		std::unordered_map<uint64_t, VkPipelineLayout> m_layouts;
	};
}
