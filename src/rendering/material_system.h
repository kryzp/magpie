#ifndef MATERIAL_SYSTEM_H_
#define MATERIAL_SYSTEM_H_

#include "container/string.h"
#include "container/hash_map.h"

#include "vulkan/descriptor_allocator.h"
#include "vulkan/descriptor_layout_cache.h"

#include "material.h"
#include "bindless_resource_mgr.h"

namespace llt
{
	class MaterialRegistry
	{
	public:
		MaterialRegistry() = default;
		~MaterialRegistry() = default;

		void loadDefaultTechniques();

		void cleanUp();

		Material *buildMaterial(MaterialData &data);
		void addTechnique(const String &name, const Technique &technique);

	private:
		HashMap<uint64_t, Material*> m_materials;
		HashMap<String, Technique> m_techniques;
	};

	class MaterialSystem
	{
	public:
		MaterialSystem();
		~MaterialSystem();

		void init();
		void finalise();

		MaterialRegistry &getRegistry();
		const MaterialRegistry &getRegistry() const;

		Texture *getIrradianceMap() const;
		Texture *getPrefilterMap() const;

		Texture *getBRDFLUT() const;

		Texture *getDiffuseFallback() const;
		Texture *getAOFallback() const;
		Texture *getRoughnessMetallicFallback() const;
		Texture *getNormalFallback() const;
		Texture *getEmissiveFallback() const;

	private:
		void generateEnvironmentMaps(CommandBuffer &cmd);
		void precomputeBRDF(CommandBuffer &cmd);

		MaterialRegistry m_registry;

		DescriptorPoolDynamic m_descriptorPoolAllocator;

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
