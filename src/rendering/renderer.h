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

		Vector<RenderObject>::Iterator createRenderObject();

	private:
		void createQuadMesh();
		void createSkybox();
		void addRenderObjects();
		void createPostProcessResources();

		void renderSkybox(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target);
		void renderObjects(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target);
		void renderParticles(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target, float deltaTime);
		void renderPostProcess();
		void renderImGui(CommandBuffer &buffer);

		void aggregateSubMeshes(Vector<SubMesh*>& list);
		void sortRenderListByMaterialHash(int lo, int hi);

		SubMesh m_quadMesh;
		SubMesh m_skyboxMesh;

//		RenderTarget *m_gBuffer;

		Vector<RenderObject> m_renderObjects;
		Vector<SubMesh*> m_renderList;

		Material *m_skyboxMaterial;

//		GraphicsPipeline m_postProcessPipeline;
//		VkDescriptorSet m_postProcessDescriptorSet;

//		DescriptorPoolDynamic m_descriptorPool;

//		GPUParticles m_gpuParticles;
	};
}

#endif // RENDERER_H_
