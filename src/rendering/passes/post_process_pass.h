#ifndef POST_PROCESS_PASS_H_
#define POST_PROCESS_PASS_H_

#include "vulkan/pipeline.h"

namespace llt
{
	class CommandBuffer;
	class GenericRenderTarget;

	class PostProcessPass
	{
	public:
		PostProcessPass() = default;
		~PostProcessPass() = default;

		void init(DescriptorPoolDynamic &pool);
		void cleanUp();

		void render(CommandBuffer &cmd);

		float getExposure() const;
		void setExposure(float exposure);

		float getBloomRadius() const;
		void setBloomRadius(float radius);

		float getBloomIntensity() const;
		void setBloomIntensity(float radius);

	private:
		void initDefaultValues();

		void createHDRResources(DescriptorPoolDynamic &pool);
		void createBloomResources(DescriptorPoolDynamic &pool);

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
		VkDescriptorSet m_bloomUpsampleSet;

		RenderTarget *m_bloomTarget;
	};

	extern PostProcessPass g_postProcessPass;
}

#endif // POST_PROCESS_PASS_H_
