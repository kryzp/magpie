#ifndef VK_SHADER_H_
#define VK_SHADER_H_

#include <glm/glm.hpp>

#include "third_party/volk.h"

#include "container/hash_map.h"
#include "container/vector.h"
#include "container/string.h"
#include "container/pair.h"

namespace llt
{
	class VulkanCore;

	class ShaderProgram
	{
	public:
		ShaderProgram();
		~ShaderProgram();

		void cleanUp();

		void loadFromSource(const char *source, uint64_t size);

		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

		VkShaderStageFlagBits getStage() const;
		void setStage(VkShaderStageFlagBits stage);

		VkShaderModule getModule() const;

	private:
		VkShaderStageFlagBits m_stage;
		VkShaderModule m_module;
	};

	class ShaderEffect
	{
	public:
		ShaderEffect();
		~ShaderEffect() = default;

		void addStage(ShaderProgram *program);

		const Vector<ShaderProgram *> &getStages() const;
		const ShaderProgram *getStage(int idx) const;

		const VkDescriptorSetLayout &getDescriptorSetLayout() const;
		void setDescriptorSetLayout(const VkDescriptorSetLayout &layout);
		
		void setPushConstantsSize(uint64_t size);
		uint64_t getPushConstantsSize() const;

	private:
		Vector<ShaderProgram *> m_stages;

		VkDescriptorSetLayout m_descriptorSetLayout;
		uint64_t m_pushConstantsSize;
	};
}

#endif // VK_SHADER_H_
