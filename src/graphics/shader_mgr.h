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

		ShaderProgram* create(const String& source, VkShaderStageFlagBits type);

		ShaderEffect* createEffect();

	private:
		HashMap<uint64_t, ShaderProgram*> m_shaderCache;
		Vector<ShaderEffect*> m_effects;
	};

	extern ShaderMgr* g_shaderManager;
}

#endif // VK_SHADER_MGR_H_
