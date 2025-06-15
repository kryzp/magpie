#pragma once

#include <string>
#include <unordered_map>

#include "graphics/render_graph.h"

#include "material.h"

namespace mgp
{
	class Image;
	class ImageView;
	class Sampler;
	class GPUBuffer;
	class CommandBuffer;
	class Swapchain;
	class Descriptor;
	class DescriptorLayout;
	class DescriptorPool;

	class Camera;
	class Scene;
	class App;
	class Mesh;

	struct GBuffer
	{
		Image *position, *albedo, *normal, *material, *emissive, *lighting, *depth;
	};

	struct EnvironmentProbe
	{
		Image *prefilter, *irradiance;
	};

	struct RenderContext
	{
		CommandBuffer *cmd;
		Swapchain *swapchain;
		Scene *scene;
		Camera *camera;
	};

	class Renderer
	{
	public:
		Renderer();
		~Renderer() = default;

		void init(App *app);
		void destroy();
		void render(const RenderContext &context);
		
		Material *buildMaterial(const MaterialData &data);

	private:
		void createGBuffer();
		void createSkyboxResources();

		void precomputeBRDF_LUT();
		void generateEnvironmentMaps();

		void loadTechniques();
		void addTechnique(const std::string &name, const Technique &technique);

		Descriptor *allocateDescriptor(const std::vector<DescriptorLayout *> &layouts);

		// world
		void shadowPass(const RenderContext &context);
		void deferredPass(const RenderContext &context);
		void lightingPass(const RenderContext &context);

		// post-processing
		void tonemappingPass(float exposure);

		// utils
		ImageView *stdView(Image *image);

		uint32_t smpIdx(Sampler *sampler);
		uint32_t tex2DIdx(ImageView *view);
		uint32_t cbmIdx(ImageView *cubemap);

		App *m_app;

		RenderGraph *m_renderGraph;

		GBuffer m_gBuffer;

		GPUBuffer *m_frameConstantsBuffer;
		GPUBuffer *m_transformDataBuffer;
		GPUBuffer *m_bindlessMaterialTable;
		GPUBuffer *m_pointLightBuffer;
		GPUBuffer *m_bufferPointersTable;

		DescriptorPool *m_descriptorPool;

		Descriptor *m_textureUV_descriptor;
		Descriptor *m_hdrTonemapping_descriptor;

		Image *m_brdfLUT;

		Image *m_environmentMap;
		EnvironmentProbe m_environmentProbe;

		Mesh *m_skyboxMesh;
		Descriptor *m_skybox_descriptor;

		std::unordered_map<uint64_t, Material *> m_materials;
		std::unordered_map<std::string, Technique> m_techniques;
		uint32_t m_materialFreeIndex;
	};
}
