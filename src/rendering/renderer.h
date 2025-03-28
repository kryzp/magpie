#ifndef RENDERER_H_
#define RENDERER_H_

#include "scene.h"

#include "vulkan/descriptor_allocator.h"
#include "vulkan/descriptor_layout_cache.h"

namespace llt
{
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
