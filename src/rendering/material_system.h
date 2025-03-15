#ifndef MATERIAL_SYSTEM_H_
#define MATERIAL_SYSTEM_H_

#include "container/string.h"
#include "container/hash_map.h"

#include "vulkan/descriptor_allocator.h"
#include "vulkan/descriptor_layout_cache.h"

#include "shader_buffer_mgr.h"

#include "material.h"
#include "light.h"

namespace llt
{
	class MaterialSystem
	{
	public:
		MaterialSystem();
		~MaterialSystem();

		void init();
		void loadDefaultTechniques();

		Material *buildMaterial(MaterialData &data);

		void addTechnique(const String &name, const Technique &technique);

		void pushGlobalData();
		void pushInstanceData();

		DynamicShaderBuffer *getGlobalBuffer() const;
		DynamicShaderBuffer *getInstanceBuffer() const;

		struct
		{
			glm::mat4 proj;
			glm::mat4 view;
			glm::vec4 cameraPosition;
			Light lights[16];
		}
		m_globalData;

		struct
		{
			glm::mat4 model;
			glm::mat4 normalMatrix;
		}
		m_instanceData;

		const Texture *getDiffuseFallback() const;
		const Texture *getAOFallback() const;
		const Texture *getRoughnessMetallicFallback() const;
		const Texture *getNormalFallback() const;
		const Texture *getEmissiveFallback() const;

	private:
		void generateEnvironmentMaps(CommandBuffer &cmd);
		void precomputeBRDF(CommandBuffer &cmd);

		HashMap<uint64_t, Material*> m_materials;
		HashMap<String, Technique> m_techniques;

		DynamicShaderBuffer *m_globalBuffer;
		DynamicShaderBuffer *m_instanceBuffer;

		DescriptorPoolDynamic m_descriptorPoolAllocator;

//		ShaderParameters m_bindlessParams;
//		DescriptorBuilder m_bindlessDescriptor;
//
//		DescriptorPoolDynamic bindlessDescriptorPoolManager;
//		DescriptorCache bindlessDescriptorCache;

//		GPUBuffer *m_bindlessUBO;
//		GPUBuffer *m_bindlessSSBO;
//		GPUBuffer *m_bindlessCombinedSamplers;

		Texture *m_environmentMap;
		Texture *m_irradianceMap;
		Texture *m_prefilterMap;
		Texture *m_brdfLUT;

		GraphicsPipelineDefinition m_equirectangularToCubemapPipeline;
		GraphicsPipelineDefinition m_irradianceGenerationPipeline;
		GraphicsPipelineDefinition m_prefilterGenerationPipeline;
		GraphicsPipelineDefinition m_brdfIntegrationPipeline;
	};

	extern MaterialSystem *g_materialSystem;
}

#endif // MATERIAL_SYSTEM_H_
