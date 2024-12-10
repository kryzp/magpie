#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <vulkan/vulkan.h>

#include "../container/vector.h"

#include "descriptor_builder.h"

namespace llt
{
	class ShaderProgram;
	class Texture;
	class TextureSampler;
	class ShaderBuffer;

	class Pipeline
	{
	public:
		Pipeline(VkShaderStageFlagBits stage);
		virtual ~Pipeline() = default;

		virtual VkPipeline getPipeline() = 0;

		VkPipelineLayout getPipelineLayout();
		VkDescriptorSet getDescriptorSet();
		Vector<uint32_t> getDynamicOffsets();

		virtual void bindShader(const ShaderProgram* shader) = 0;
		void bindTexture(int idx, const Texture* texture, TextureSampler* sampler);
		void bindBuffer(int idx, const ShaderBuffer* buffer);

	protected:
		Vector<const Texture*> p_boundImagesTextures;

	private:
		void sortBoundOffsets(Vector<Pair<uint32_t, uint32_t>>& offsets, int lo, int hi);

		DescriptorBuilder m_descriptorBuilder;

		VkShaderStageFlagBits m_stage;

		Vector<VkDescriptorImageInfo> m_boundImages;

		Vector<Pair<int, const ShaderBuffer*>> m_buffers;
	};
}

#endif // PIPELINE_H_
