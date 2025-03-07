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

	class Pipeline
	{
	public:
		Pipeline();
		~Pipeline() = default;

		VkPipeline buildGraphicsPipeline(const RenderInfo &renderInfo);
		VkPipeline buildComputePipeline();

		VkPipeline getPipeline();
		
		VkPipelineBindPoint getBindPoint() const;

		void bindShader(ShaderEffect *shader);

		void setVertexFormat(const VertexFormat &format);

		void setSampleShading(bool enabled, float minSampleShading);
		void setCullMode(VkCullModeFlagBits cull);

		void setDepthOp(VkCompareOp op);
		void setDepthTest(bool enabled);
		void setDepthWrite(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

	private:
		VkPipeline m_pipeline;

		VkPipelineBindPoint m_bindPoint;

		VkPipelineDepthStencilStateCreateInfo m_depthStencilCreateInfo;

		bool m_sampleShadingEnabled;
		float m_minSampleShading;

		VkCullModeFlagBits m_cullMode;

		VertexFormat m_currentVertexFormat;

		VkPipelineShaderStageCreateInfo m_computeShaderStageInfo;

		ShaderEffect *m_boundShader;
		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_shaderStages;
	};
}

#endif // PIPELINE_H_
