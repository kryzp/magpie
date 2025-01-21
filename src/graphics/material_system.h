#ifndef MATERIAL_SYSTEM_H_
#define MATERIAL_SYSTEM_H_

#include "../container/string.h"
#include "../container/hash_map.h"

#include "descriptor_allocator.h"
#include "descriptor_layout_cache.h"
#include "shader_buffer_mgr.h"
#include "material.h"

namespace llt
{
	class MaterialSystem
	{
	public:
		MaterialSystem();
		~MaterialSystem();

		void init();
		void loadDefaultTechniques();

		Material* buildMaterial(MaterialData& data);

		void addTechnique(const String& name, const Technique& technique);

		DynamicShaderBuffer* getGlobalBuffer() const;
		DynamicShaderBuffer* getInstanceBuffer() const;

	private:
		HashMap<uint64_t, Material*> m_materials;
		HashMap<String, Technique> m_techniques;

		DynamicShaderBuffer* m_globalBuffer;
		DynamicShaderBuffer* m_instanceBuffer;

		DescriptorPoolDynamic m_descriptorPoolAllocator;
		DescriptorLayoutCache m_descriptorCache;

//		ShaderParameters m_bindlessParams;
//		DescriptorBuilder m_bindlessDescriptor;
//
//		DescriptorPoolDynamic bindlessDescriptorPoolManager;
//		DescriptorCache bindlessDescriptorCache;

		GPUBuffer* m_bindlessUBO;
		GPUBuffer* m_bindlessSSBO;
		GPUBuffer* m_bindlessCombinedSamplers;
	};

	extern MaterialSystem* g_materialSystem;
}

#endif // MATERIAL_SYSTEM_H_
