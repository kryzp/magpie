#ifndef VK_SHADER_MGR_H_
#define VK_SHADER_MGR_H_

#include <vulkan/vulkan.h>

#include "shader.h"

#include "../container/vector.h"
#include "../container/hash_map.h"

namespace llt
{
	class VulkanBackend;
	
	class ShaderMgr
	{
	public:
		ShaderMgr();
		~ShaderMgr();

		ShaderProgram* get(const String& name);
		ShaderProgram* create(const String& name, const String& source, VkShaderStageFlagBits type);

//		ShaderEffect* createEffect();

	private:
		HashMap<String, ShaderProgram*> m_shaderCache;
//		Vector<ShaderEffect*> m_effects;
	};

	extern ShaderMgr* g_shaderManager;
}

#endif // VK_SHADER_MGR_H_
