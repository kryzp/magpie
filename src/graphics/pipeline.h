#ifndef PIPELINE_H_
#define PIPELINE_H_

#include <vulkan/vulkan.h>

#include "../container/vector.h"

#include "descriptor_builder.h"
#include "descriptor_layout_cache.h"
#include "descriptor_allocator.h"

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
		virtual ~Pipeline();

		virtual void bind() = 0;
		virtual VkPipeline getPipeline() = 0;

		VkPipelineLayout getPipelineLayout();

		void setDescriptorSetLayout(const VkDescriptorSetLayout& layout);

		virtual void bindShader(const ShaderProgram* shader) = 0;

	private:
		VkShaderStageFlagBits m_stage;
		VkDescriptorSetLayout m_descriptorSetLayout;
	};
}

#endif // PIPELINE_H_
