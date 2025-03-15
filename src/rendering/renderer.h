#ifndef RENDERER_H_
#define RENDERER_H_

#include "container/vector.h"

#include "vulkan/vertex_format.h"
#include "vulkan/shader.h"
#include "vulkan/pipeline.h"
#include "vulkan/command_buffer.h"

#include "sub_mesh.h"
#include "render_object.h"
#include "gpu_particles.h"
#include "scene.h"

namespace llt
{
	class Swapchain;
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

		void renderSkybox(CommandBuffer &cmd, const Camera &camera);
		void renderImGui(CommandBuffer &cmd);

		RenderTarget *m_target;

		Scene m_currentScene;

		SubMesh m_skyboxMesh;
		GraphicsPipelineDefinition m_skyboxPipeline;
		VkDescriptorSet m_skyboxSet;

		DescriptorPoolDynamic m_descriptorPool;
		DescriptorLayoutCache m_descriptorLayoutCache;
	};
}

#endif // RENDERER_H_
