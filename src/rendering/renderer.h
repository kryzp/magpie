#pragma once

#include <unordered_map>
#include <string>

#include "shadow_map_atlas.h"
#include "material.h"

#include "vulkan/pipeline_cache.h"
#include "vulkan/descriptor.h"
#include "vulkan/bindless.h"

namespace mgp
{
	class App;
	class Image;
	class Camera;
	class Scene;
	class GPUBuffer;
	class Shader;
	class Mesh;
	class Swapchain;

	struct GBuffer
	{
		Image *position;
		Image *albedo;
		Image *normal;
		Image *material;
		Image *emissive;
		
		Image *lighting;

		Image *depth;
	};

	struct EnvironmentProbe
	{
		Image *prefilter;
		Image *irradiance;
	};

	struct RenderContext
	{
		// scene
		// camera
	};

	class Renderer
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		void init(App *app, Swapchain *swapchain);
		void destroy();

		void render(Scene &scene, const Camera &camera, Swapchain *swapchain);
		
		Material *buildMaterial(MaterialData &data);
		
		VkDescriptorSet allocateSet(const std::vector<VkDescriptorSetLayout> &layouts);
		
	private:
		void shadowPass(Scene &scene, const Camera &camera);
		void deferredPass(Scene &scene, const Camera &camera);
		void lightingPass(Scene &scene, const Camera &camera);
		void tonemappingPass(float exposure);

		void createGBuffer(Swapchain *swapchain);

		void createSkyboxMesh();
		void precomputeBRDF();
		void generateEnvironmentProbe();
		
		void loadTechniques();
		void addTechnique(const std::string &name, const Technique &technique);

		App *m_app;

		GBuffer m_gBuffer;

		GPUBuffer *m_frameConstantsBuffer;
		GPUBuffer *m_transformDataBuffer;
		GPUBuffer *m_bindlessMaterialTable;
		GPUBuffer *m_pointLightBuffer;
		GPUBuffer *m_bufferPointersBuffer;

		std::unordered_map<uint64_t, Material *> m_materials;
		std::unordered_map<std::string, Technique> m_techniques;
		uint32_t m_materialHandle_UID;

		VkDescriptorSet m_textureUVSet;
		VkDescriptorSet m_hdrTonemappingSet;

		DescriptorPoolDynamic m_descriptorPool;

		Image *m_brdfLUT;

//		ShadowMapAtlas m_shadowAtlas;

		Image *m_environmentMap;
		EnvironmentProbe m_environmentProbe;

		Mesh *m_skyboxMesh;
		VkDescriptorSet m_skyboxSet;
	};
}
