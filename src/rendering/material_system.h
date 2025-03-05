#ifndef MATERIAL_SYSTEM_H_
#define MATERIAL_SYSTEM_H_

#include "container/string.h"
#include "container/hash_map.h"

#include "vulkan/descriptor_allocator.h"
#include "vulkan/descriptor_layout_cache.h"

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

		void updateImGui();

		Material *buildMaterial(MaterialData &data);

		void addTechnique(const String &name, const Technique &technique);

		DynamicShaderBuffer *getGlobalBuffer() const;
		DynamicShaderBuffer *getInstanceBuffer() const;

	private:
		void createQuadMesh();
		void createCubeMesh();

		void generateEnvironmentMaps(CommandBuffer &buffer);

		void precomputeBRDF(CommandBuffer &buffer);

		HashMap<uint64_t, Material*> m_materials;
		HashMap<String, Technique> m_techniques;

		DynamicShaderBuffer *m_globalBuffer;
		DynamicShaderBuffer *m_instanceBuffer;

		DescriptorPoolDynamic m_descriptorPoolAllocator;
		DescriptorLayoutCache m_descriptorCache;

//		ShaderParameters m_bindlessParams;
//		DescriptorBuilder m_bindlessDescriptor;
//
//		DescriptorPoolDynamic bindlessDescriptorPoolManager;
//		DescriptorCache bindlessDescriptorCache;

		GPUBuffer *m_bindlessUBO;
		GPUBuffer *m_bindlessSSBO;
		GPUBuffer *m_bindlessCombinedSamplers;

		Texture *m_environmentMap;
		Texture *m_irradianceMap;
		Texture *m_prefilterMap;
		Texture *m_brdfIntegration;

		Pipeline m_equirectangularToCubemapPipeline;
		Pipeline m_irradianceGenerationPipeline;
		Pipeline m_prefilterGenerationPipeline;
		Pipeline m_brdfIntegrationPipeline;

		SubMesh *m_quadMesh;
		SubMesh *m_cubeMesh;
	};

	extern MaterialSystem *g_materialSystem;
}

#endif // MATERIAL_SYSTEM_H_
