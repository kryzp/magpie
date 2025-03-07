#ifndef RENDERER_H_
#define RENDERER_H_

#include "container/vector.h"

#include "vulkan/vertex_format.h"
#include "vulkan/shader.h"
#include "vulkan/pipeline.h"
#include "vulkan/command_buffer.h"

#include "passes/forward_pass.h"
#include "passes/post_process_pass.h"

#include "sub_mesh.h"
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

		void renderSkybox(CommandBuffer &buffer, const Camera &camera);
		void renderImGui(CommandBuffer &buffer);

		RenderTarget *m_target;

		Scene m_currentScene;

		SubMesh m_skyboxMesh;
		Pipeline m_skyboxPipeline;
		VkDescriptorSet m_skyboxSet;

		ForwardPass m_forwardPass;
		PostProcessPass m_postProcessPass;

		DescriptorPoolDynamic m_descriptorPool;
		DescriptorLayoutCache m_descriptorLayoutCache;

//		GraphicsPipeline m_postProcessPipeline;
//		VkDescriptorSet m_postProcessDescriptorSet;

//		DescriptorPoolDynamic m_descriptorPool;

//		GPUParticles m_gpuParticles;
	};
}

#endif // RENDERER_H_
