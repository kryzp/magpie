#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "../third_party/volk.h"

#include "../common.h"

#include "../container/array.h"
#include "../container/optional.h"
#include "../container/vector.h"

#include "../math/rect.h"

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
	class ShaderProgram;
	class TextureSampler;
	class DynamicShaderBuffer;
	struct RenderPass;
	class GenericRenderTarget;
	class RenderInfo;

	class Pipeline
	{
	public:
		static Pipeline fromGraphics();
		static Pipeline fromCompute();

		Pipeline(VkShaderStageFlagBits stage, VkPipelineBindPoint bindPoint);
		virtual ~Pipeline();

		VkPipeline buildGraphicsPipeline(RenderInfo *renderInfo);
		VkPipeline buildComputePipeline();

		VkPipeline getPipeline();
		VkPipelineLayout getPipelineLayout();
		
		VkPipelineBindPoint getBindPoint() const;

		void bindShader(const ShaderProgram *shader);
		void setPushConstantsSize(uint32_t size);

		void setDescriptorSetLayout(const VkDescriptorSetLayout &layout);

		void setVertexFormat(const VertexFormat &format);

		void setMSAA(VkSampleCountFlagBits samples);
		void setSampleShading(bool enabled, float minSampleShading);
		void setCullMode(VkCullModeFlagBits cull);

		void setDepthOp(VkCompareOp op);
		void setDepthTest(bool enabled);
		void setDepthWrite(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

	private:
		VkPipeline m_pipeline;

		VkShaderStageFlagBits m_stage;
		VkDescriptorSetLayout m_descriptorSetLayout;
		VkPipelineBindPoint m_bindPoint;
		uint32_t m_pushConstantsSize;

		VkPipelineDepthStencilStateCreateInfo m_depthStencilCreateInfo;

		VkSampleCountFlagBits m_samples;
		bool m_sampleShadingEnabled;
		float m_minSampleShading;

		VkCullModeFlagBits m_cullMode;

		VertexFormat m_currentVertexFormat;

		VkPipelineShaderStageCreateInfo m_computeShaderStageInfo;
		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_shaderStages;
	};
}

#endif // PIPELINE_H_
