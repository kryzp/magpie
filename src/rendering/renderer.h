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

	class GBuffer
	{
	};

	class Renderer
	{
		struct EnvironmentProbe
		{
			Image *prefilter;
			Image *irradiance;
		};

	public:
		Renderer() = default;
		~Renderer() = default;

		void init(App *app, Swapchain *swapchain);
		void destroy();

		void render(Scene &scene, const Camera &camera, Swapchain *swapchain);
		
		Material *buildMaterial(MaterialData &data);

	private:
		void shadowPass(Scene &scene, const Camera &camera);
		void deferredPass(Scene &scene, const Camera &camera);

		void precomputeBRDF();
		void generateEnvironmentProbe();

		void createSkyboxMesh();
		void createSkybox();
		
		void loadTechniques();
		void addTechnique(const std::string &name, const Technique &technique);

		App *m_app;

		Image *m_targetColour;
		Image *m_targetDepth;

		GPUBuffer *m_frameConstantsBuffer;
		GPUBuffer *m_transformDataBuffer;
		GPUBuffer *m_bindlessMaterialTable;
		GPUBuffer *m_pointLightBuffer;
		GPUBuffer *m_bufferPointersBuffer;

		std::unordered_map<uint64_t, Material *> m_materials;
		std::unordered_map<std::string, Technique> m_techniques;
		uint32_t m_materialHandle_UID;

		GraphicsPipelineDef m_textureUVPipeline;
		VkDescriptorSet m_textureUVSet;

		ComputePipelineDef m_hdrTonemappingPipeline;
		VkDescriptorSet m_hdrTonemappingSet;

		Image *m_brdfLUT;

		ShadowMapAtlas m_shadowAtlas;

		Image *m_environmentMap;
		EnvironmentProbe m_environmentProbe;

		Mesh *m_skyboxMesh;
		Shader *m_skyboxShader;
		VkDescriptorSet m_skyboxSet;
		GraphicsPipelineDef m_skyboxPipeline;
	};
}
