#pragma once

#include <vector>

#include <Volk/volk.h>

namespace mgp
{
	class VulkanCore;

	class ShaderStage
	{
	public:
		ShaderStage(const VulkanCore *core);
		ShaderStage(const VulkanCore *core, VkShaderStageFlagBits stage, const char *path);
		~ShaderStage();

		void compileFromSource(const char *path);

		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

		VkShaderStageFlagBits getStage() const { return m_stage; }
		void setStage(VkShaderStageFlagBits stage) { m_stage = stage; }

		VkShaderModule getModule() const { return m_module; }

	private:
		VkShaderStageFlagBits m_stage;
		VkShaderModule m_module;

		const VulkanCore *m_core;
	};

	class Shader
	{
	public:
		Shader();
		~Shader() = default;

		void addStage(ShaderStage *stage);

		const std::vector<ShaderStage *> &getStages() const;
		const ShaderStage *getStage(int idx) const;

		const std::vector<VkDescriptorSetLayout> &getDescriptorSetLayouts() const;
		void setDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &layouts);

		void setPushConstantsSize(uint64_t size);
		uint64_t getPushConstantsSize() const;

	private:
		std::vector<ShaderStage *> m_stages;
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
		uint64_t m_pushConstantsSize;
	};
}
