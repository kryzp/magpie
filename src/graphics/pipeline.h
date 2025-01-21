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
		Pipeline(VkShaderStageFlagBits stage);
		virtual ~Pipeline();

		virtual void bind() = 0;

		void preCache();
		virtual VkPipeline getPipeline() = 0;

		VkPipelineLayout getPipelineLayout();

		void setDescriptorSetLayout(const VkDescriptorSetLayout& layout);

		virtual void bindShader(const ShaderProgram* shader) = 0;

	private:
		VkShaderStageFlagBits m_stage;
		VkDescriptorSetLayout m_descriptorSetLayout;
	};

	class GraphicsPipeline : public Pipeline
	{
	public:
		GraphicsPipeline();
		~GraphicsPipeline() override;

		void render(const RenderPass& op);

		void bind() override;

		VkPipeline getPipeline() override;

		void bindShader(const ShaderProgram* shader) override;

		void setVertexFormat(const VertexFormat& format);

		void setSampleShading(bool enabled, float minSampleShading);
		void setCullMode(VkCullModeFlagBits cull);

		void setDepthOp(VkCompareOp op);
		void setDepthTest(bool enabled);
		void setDepthWrite(bool enabled);
		void setDepthBounds(float min, float max);
		void setDepthStencilTest(bool enabled);

		void setViewport(const RectF& rect);
		void setScissor(const RectI& rect);

		void resetViewport();
		void resetScissor();

	private:
		VkViewport getViewport() const;
		VkRect2D getScissor() const;

		VkPipelineDepthStencilStateCreateInfo m_depthStencilCreateInfo;

		bool m_sampleShadingEnabled;
		float m_minSampleShading;

		VkCullModeFlagBits m_cullMode;

		Optional<VkViewport> m_viewport;
		Optional<VkRect2D> m_scissor;

		VertexFormat m_currentVertexFormat;

		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_shaderStages;
	};

	class ComputePipeline : public Pipeline
	{
	public:
		ComputePipeline();
		~ComputePipeline() override;

		void dispatch(int gcX, int gcY, int gcZ);

		void bind() override;
		VkPipeline getPipeline() override;

		void bindShader(const ShaderProgram* shader) override;

	private:
		VkPipelineShaderStageCreateInfo m_computeShaderStageInfo;
	};
}

#endif // PIPELINE_H_
