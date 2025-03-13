#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "third_party/volk.h"

#include "core/common.h"

#include "container/array.h"
#include "container/optional.h"
#include "container/vector.h"

#include "math/rect.h"

#include "descriptor_builder.h"
#include "descriptor_layout_cache.h"
#include "descriptor_allocator.h"
#include "vertex_format.h"
#include "blend.h"
#include "queue.h"
#include "texture.h"
#include "render_target.h"

namespace llt
{
	class ShaderEffect;
	class TextureSampler;
	class DynamicShaderBuffer;
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

	struct PipelineData
	{
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	class PipelineCache
	{
	public:
		PipelineCache();
		~PipelineCache();

		void init();
		void dispose();

		PipelineData fetchGraphicsPipeline(const GraphicsPipelineDefinition &definition, const RenderInfo &renderInfo);
		PipelineData fetchComputePipeline(const ComputePipelineDefinition &definition);

		VkPipelineLayout fetchPipelineLayout(const ShaderEffect *shader);

	private:
		HashMap<uint64_t, VkPipeline> m_pipelines;
		HashMap<uint64_t, VkPipelineLayout> m_layouts;
	};
}

#endif // PIPELINE_H_
