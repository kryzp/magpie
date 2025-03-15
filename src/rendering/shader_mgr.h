#ifndef VK_SHADER_MGR_H_
#define VK_SHADER_MGR_H_

#include "third_party/volk.h"

#include "vulkan/shader.h"
#include "vulkan/descriptor_layout_cache.h"

#include "container/vector.h"
#include "container/hash_map.h"

namespace llt
{
	class VulkanCore;
	
	class ShaderMgr
	{
	public:
		ShaderMgr();
		~ShaderMgr();

		void loadDefaultShaders();

		ShaderProgram *get(const String &name);
		ShaderProgram *load(const String &name, const String &source, VkShaderStageFlagBits stage);

		ShaderEffect *getEffect(const String &name);
		ShaderEffect *createEffect(const String &name);

	private:
		void loadDefaultShaderPrograms();
		void createDefaultShaderEffects();

		HashMap<String, ShaderProgram*> m_shaderCache;
		HashMap<String, ShaderEffect*> m_effects;
	};

	extern ShaderMgr *g_shaderManager;
}

#endif // VK_SHADER_MGR_H_
