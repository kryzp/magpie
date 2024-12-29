#ifndef MATERIAL_SYSTEM_H_
#define MATERIAL_SYSTEM_H_

#include "../container/string.h"
#include "../container/hash_map.h"

#include "shader_buffer_mgr.h"
#include "material.h"

namespace llt
{
	class MaterialSystem
	{
	public:
		MaterialSystem();
		~MaterialSystem();

		void initBuffers();
		void loadDefaultTechniques();

		Material* buildMaterial(MaterialData& data);

		void addTechnique(const String& name, const Technique& technique);

		ShaderParameters globalParameters;
		ShaderParameters instanceParameters;

		void updateGlobalBuffer();
		void updateInstanceBuffer();

		const ShaderBuffer* getGlobalBuffer() const;
		const ShaderBuffer* getInstanceBuffer() const;

	private:
		HashMap<uint64_t, Material*> m_materials;
		HashMap<String, Technique> m_techniques;

		ShaderBuffer* m_globalBuffer;
		ShaderBuffer* m_instanceBuffer;
	};

	extern MaterialSystem* g_materialSystem;
}

#endif // MATERIAL_SYSTEM_H_
