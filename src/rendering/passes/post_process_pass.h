#ifndef POST_PROCESS_PASS_H_
#define POST_PROCESS_PASS_H_

#include "vulkan/pipeline_definition.h"
#include "vulkan/texture_view.h"

namespace llt
{
	class CommandBuffer;
	class GenericRenderTarget;
	class RenderTarget;
	class DescriptorPoolDynamic;

	class PostProcessPass
	{
		constexpr static int BLOOM_MIPS = 6;

	public:
		PostProcessPass() = default;
		~PostProcessPass() = default;

		void init(DescriptorPoolDynamic &pool, RenderTarget *input);
		void dispose();

		void render(CommandBuffer &cmd);

		float getExposure() const;
		void setExposure(float exposure);

		float getBloomRadius() const;
		void setBloomRadius(float radius);

		float getBloomIntensity() const;
		void setBloomIntensity(float radius);

	private:
		void initDefaultValues();

		void createHDRResources(DescriptorPoolDynamic &pool, RenderTarget *input);
		void createBloomResources(DescriptorPoolDynamic &pool, RenderTarget *input);

		void applyHDRTexture(CommandBuffer &cmd);

		void renderBloomDownsamples(CommandBuffer &cmd);
		void renderBloomUpsamples(CommandBuffer &cmd);

		float m_exposure;
		float m_bloomRadius;
		float m_bloomIntensity;

		GraphicsPipelineDefinition m_hdrPipeline;
		VkDescriptorSet m_hdrSet;

		GraphicsPipelineDefinition m_bloomDownsamplePipeline;
		VkDescriptorSet m_bloomDownsampleSet;

		GraphicsPipelineDefinition m_bloomUpsamplePipeline;
		VkDescriptorSet m_bloomUpsampleSets[BLOOM_MIPS - 1];

		TextureView m_bloomViews[BLOOM_MIPS];

		RenderTarget *m_bloomTarget;
	};

	extern PostProcessPass g_postProcessPass;
}

#endif // POST_PROCESS_PASS_H_
