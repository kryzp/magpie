#pragma once

#include <vector>

#include <Volk/volk.h>

// todo
// yes this is cluttering the global namespace
// in the future i want to move a lot of stuff like this
// into an interface system
// will also make abstracting vulkan away a lot easier
//#include <spirv_cross/spirv_cross.hpp>

namespace mgp
{
	class VulkanCore;

	class ShaderStage
	{
	public:
		ShaderStage(VulkanCore *core);
		ShaderStage(VulkanCore *core, VkShaderStageFlagBits stage, const char *path);
		~ShaderStage();

		void compileFromSource(const char *path);

		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

		VkShaderStageFlagBits getType() const { return m_stage; }
		void setStage(VkShaderStageFlagBits stage) { m_stage = stage; }

		VkShaderModule getModule() const { return m_module; }

//		const spirv_cross::Compiler *getCompiler() const { return m_compiler; };
//		const spirv_cross::ShaderResources &getResources() const { return m_resources; }

	private:
		VkShaderStageFlagBits m_stage;
		VkShaderModule m_module;

//		spirv_cross::Compiler *m_compiler;
//		spirv_cross::ShaderResources m_resources;

		VulkanCore *m_core;
	};

	class Shader
	{
	public:
		Shader(VulkanCore *core);
		~Shader() = default;

		void addStage(ShaderStage *stage);
		void finalize();

		const std::vector<ShaderStage *> &getStages() const;
		const ShaderStage *getStage(int idx) const;

		const std::vector<VkDescriptorSetLayout> &getDescriptorSetLayouts() const;
		void setDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &layouts);
		
		void setPushConstantsSize(uint64_t size);
		uint64_t getPushConstantsSize() const;

	private:
		VulkanCore *m_core;

		std::vector<ShaderStage *> m_stages;
		std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
		uint64_t m_pushConstantsSize;
	};
}
