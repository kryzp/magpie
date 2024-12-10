#ifndef GRAPHICS_PIPELINE_H_
#define GRAPHICS_PIPELINE_H_

#include "../common.h"
#include "../container/array.h"
#include "../container/optional.h"
#include "../math/rect.h"

#include "pipeline.h"
#include "vertex_descriptor.h"
#include "blend.h"
#include "queue.h"

namespace llt
{
	class RenderOp;
	class GenericRenderTarget;
	class RenderInfoBuilder;

	class GraphicsPipeline : public Pipeline
	{
	public:
		GraphicsPipeline();
		~GraphicsPipeline() override;

		void render(const RenderOp& op);

		VkPipeline getPipeline() override;

		void bindShader(const ShaderProgram* shader) override;

		void setVertexDescriptor(const VertexDescriptor& descriptor);

		void setBlendState(const BlendState& state);

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
		Array<VkPipelineColorBlendAttachmentState, mgc::MAX_RENDER_TARGET_ATTACHMENTS> m_colourBlendAttachmentStates;
		
		float m_blendConstants[4];
		bool m_sampleShadingEnabled;
		float m_minSampleShading;
		bool m_blendStateLogicOpEnabled;
		
		VkLogicOp m_blendStateLogicOp;
		VkCullModeFlagBits m_cullMode;

		Optional<VkViewport> m_viewport;
		Optional<VkRect2D> m_scissor;
		
		const VertexDescriptor* m_currentVertexDescriptor;
		
		Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> m_graphicsShaderStages;
	};
}

#endif // GRAPHICS_PIPELINE_H_
