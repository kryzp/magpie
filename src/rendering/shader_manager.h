#pragma once

#include <Volk/volk.h>

#include <string>
#include <unordered_map>

namespace mgp
{
	class VulkanCore;
	class ShaderStage;
	class Shader;

	class ShaderManager
	{
	public:
		ShaderManager();
		~ShaderManager();

		void init(VulkanCore *core);
		void destroy();
		
		Shader *getShader(const std::string &name);
		ShaderStage *getShaderStage(const std::string &name);

		ShaderStage *loadShaderStage(const std::string &name, const std::string &path, VkShaderStageFlagBits stageType);
		
		void addShader(const std::string &name, Shader *shader);

	private:
		void loadShaders();

		VulkanCore *m_core;

		std::unordered_map<std::string, ShaderStage *> m_shaderStageCache;
		std::unordered_map<std::string, Shader *> m_shaderCache;
	};
}
