#ifndef RENDERER_H_
#define RENDERER_H_

#include "container/vector.h"

#include "vulkan/vertex_format.h"
#include "vulkan/shader.h"
#include "vulkan/pipeline.h"
#include "vulkan/command_buffer.h"

#include "rendering/sub_mesh.h"

#include "render_object.h"
#include "gpu_particles.h"
#include "scene.h"

namespace llt
{
	class Backbuffer;
	class ShaderProgram;
	class DynamicShaderBuffer;
	class Camera;

	class Renderer
	{
	public:
		Renderer();
		~Renderer() = default;

		void init();
		void cleanUp();

		void render(const Camera &camera, float deltaTime);

		void setScene(const Scene &scene);

	private:
		void createSkyboxResources();
		void addRenderObjects();
		void createPostProcessResources();

		void renderSkybox(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target);
		void renderParticles(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target, float deltaTime);
		void renderPostProcess();
		void renderImGui(CommandBuffer &buffer);

//		RenderTarget *m_gBuffer;

		Scene m_currentScene;

		SubMesh m_skyboxMesh;
		Pipeline m_skyboxPipeline;
		VkDescriptorSet m_skyboxSet;

		DescriptorPoolDynamic m_descriptorPool;
		DescriptorLayoutCache m_descriptorLayoutCache;

//		GraphicsPipeline m_postProcessPipeline;
//		VkDescriptorSet m_postProcessDescriptorSet;

//		DescriptorPoolDynamic m_descriptorPool;

//		GPUParticles m_gpuParticles;
	};
}

#endif // RENDERER_H_
