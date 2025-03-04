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
		Pipeline(VkShaderStageFlagBits stage, VkPipelineBindPoint bindPoint);
		virtual ~Pipeline();

		virtual void create(RenderInfo *renderInfo) = 0;

		void bind(CommandBuffer &buffer);

		void bindSet(CommandBuffer &buffer, const VkDescriptorSet &set);
		void bindSet(CommandBuffer &buffer, const VkDescriptorSet &set, const Vector<uint32_t> &dynamicOffsets);

		VkPipeline getPipeline();
		VkPipelineLayout getPipelineLayout();

		void setDescriptorSetLayout(const VkDescriptorSetLayout &layout);

		virtual void bindShader(const ShaderProgram *shader) = 0;

	protected:
		VkPipeline m_pipeline;

	private:
		VkShaderStageFlagBits m_stage;
		VkDescriptorSetLayout m_descriptorSetLayout;
		VkPipelineBindPoint m_bindPoint;
	};

	class GraphicsPipeline : public Pipeline
	{
	public:
		GraphicsPipeline();
		~GraphicsPipeline() override;

		void create(RenderInfo *renderInfo) override;

		void render(CommandBuffer &buffer, const RenderPass &op);
		
		void bindShader(const ShaderProgram *shader) override;

		void setVertexFormat(const VertexFormat &format);

		void setMSAA(VkSampleCountFlagBits samples);
		void setSampleShading(bool enabled, float minSampleShading);
		void setCullMode(VkCullModeFlagBits cull);

		void setDepthOp(VkCompareOp op);
		void setDepthTest(bool enabled);
		void setDepthWrite(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

		void setViewport(const RectF &rect);
		void setScissor(const RectI &rect);

		void resetViewport(const RenderInfo &info);
		void resetScissor(const RenderInfo &info);

	private:
		VkViewport getViewport() const;
		VkRect2D getScissor() const;

		VkPipelineDepthStencilStateCreateInfo m_depthStencilCreateInfo;

		VkSampleCountFlagBits m_samples;
		bool m_sampleShadingEnabled;
		float m_minSampleShading;

		VkCullModeFlagBits m_cullMode;

		VkViewport m_viewport;
		VkRect2D m_scissor;

		VertexFormat m_currentVertexFormat;

		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_shaderStages;
	};

	class ComputePipeline : public Pipeline
	{
	public:
		ComputePipeline();
		~ComputePipeline() override;

		void create(RenderInfo *renderInfo = nullptr) override;

		void dispatch(CommandBuffer &buffer, int gcX, int gcY, int gcZ);

		void bindShader(const ShaderProgram *shader) override;

	private:
		VkPipelineShaderStageCreateInfo m_computeShaderStageInfo;
	};
}

#endif // PIPELINE_H_
