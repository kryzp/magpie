#ifndef PIPELINE_DEFINITION_H_
#define PIPELINE_DEFINITION_H_

#include "third_party/volk.h"

#include "core/common.h"

#include "container/array.h"
#include "container/hash_map.h"

#include "math/rect.h"

#include "blend.h"
#include "vertex_format.h"

namespace llt
{
	class ShaderEffect;
	class TextureSampler;
	class GenericRenderTarget;
	class RenderInfo;

	class GraphicsPipelineDefinition
	{
	public:
		GraphicsPipelineDefinition();

		void setShader(const ShaderEffect *shader);
		const ShaderEffect *getShader() const;

		void setVertexFormat(const VertexFormat &format);
		const VertexFormat &getVertexFormat() const;

		void setSampleShading(bool enabled, float minSampleShading);
		
		bool isSampleShadingEnabled() const;
		float getMinSampleShading() const;

		void setCullMode(VkCullModeFlagBits cull);
		VkCullModeFlagBits getCullMode() const;

		const Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> &getShaderStages() const;
		const VkPipelineShaderStageCreateInfo &getShaderStage(int idx) const;

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

		const Array<float, 4> &getBlendConstants() const;
		float getBlendConstant(int idx) const;

	private:
		const ShaderEffect *m_shader;

		const VertexFormat *m_vertexFormat;

		VkCullModeFlagBits m_cullMode;

		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_shaderStages;

		VkPipelineDepthStencilStateCreateInfo m_depthStencilInfo;

		Array<float, 4> m_blendConstants;
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

		void setShader(const ShaderEffect *shader);
		const ShaderEffect *getShader() const;

		const VkPipelineShaderStageCreateInfo &getStage() const;

	private:
		const ShaderEffect *m_shader;
		VkPipelineShaderStageCreateInfo m_stage;
	};
}

#endif // PIPELINE_DEFINITION_H_
