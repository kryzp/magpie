#pragma once

#include <vector>
#include <string>

#include <Volk/volk.h>

namespace mgp
{
	class GraphicsCore;
	class DescriptorLayout;

	class ShaderStage
	{
	public:
		ShaderStage(GraphicsCore *gfx, VkShaderStageFlagBits type, const std::string &path);
		~ShaderStage();

		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

		VkShaderStageFlagBits getType() const { return m_stage; }

		VkShaderModule getModule() const { return m_module; }

//		const spirv_cross::Compiler *getCompiler() const { return m_compiler; };
//		const spirv_cross::ShaderResources &getResources() const { return m_resources; }

	private:
		GraphicsCore *m_gfx;

		void compileFromSource(const std::string &path);

		VkShaderStageFlagBits m_stage;
		VkShaderModule m_module;

//		spirv_cross::Compiler *m_compiler;
//		spirv_cross::ShaderResources m_resources;
	};

	class Shader
	{
	public:
		Shader(GraphicsCore *gfx, uint64_t pushConstantSize, const std::vector<DescriptorLayout *> &layouts, const std::vector<ShaderStage *> &stages);
		~Shader() = default;

		const std::vector<ShaderStage *> &getStages() const;
		uint64_t getPushConstantSize() const;
		const std::vector<DescriptorLayout *> &getLayouts() const;

	private:
		GraphicsCore *m_gfx;

		std::vector<ShaderStage *> m_stages;

		uint64_t m_pushConstantSize;
		std::vector<DescriptorLayout *> m_layouts;
	};
}
