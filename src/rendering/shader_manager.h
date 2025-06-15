#pragma once

#include <string>
#include <unordered_map>

#include "graphics/shader.h"

namespace mgp
{
	class App;

	class ShaderManager
	{
	public:
		ShaderManager() = default;
		~ShaderManager() = default;

		void init(App *app);
		void destroy();
		
		Shader *getShader(const std::string &name);
		ShaderStage *getShaderStage(const std::string &name);

		ShaderStage *loadShaderStage(const std::string &name, const std::string &path, VkShaderStageFlagBits stageType);
		
		void addShader(const std::string &name, Shader *shader);

	private:
		App *m_app;

		void loadShaders();

		std::unordered_map<std::string, ShaderStage *> m_shaderStageCache;
		std::unordered_map<std::string, Shader *> m_shaderCache;
	};
}
