#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <vulkan/vulkan.h>

#include "../container/vector.h"

#include "descriptor_builder.h"
#include "texture.h"

namespace llt
{
	class ShaderProgram;
	class TextureSampler;
	class ShaderBuffer;

	class Pipeline
	{
	public:
		Pipeline(VkShaderStageFlagBits stage);
		virtual ~Pipeline() = default;

		virtual void bind() = 0;
		virtual VkPipeline getPipeline() = 0;

		VkPipelineLayout getPipelineLayout();
		VkDescriptorSet getDescriptorSet();
		const Vector<uint32_t>& getDynamicOffsets();

		virtual void bindShader(const ShaderProgram* shader) = 0;
		void bindTexture(int idx, Texture* texture, TextureSampler* sampler);
		void bindBuffer(int idx, const ShaderBuffer* buffer);

	protected:
		TextureBatch p_boundTextures;

	private:
		DescriptorBuilder m_descriptorBuilder;

		VkShaderStageFlagBits m_stage;

		Vector<VkDescriptorImageInfo> m_boundImages;

		Vector<uint32_t> m_dynamicOffsets;
		Vector<Pair<int, const ShaderBuffer*>> m_buffers;
	};
}

#endif // PIPELINE_H_
