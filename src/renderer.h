#ifndef RENDERER_H_
#define RENDERER_H_

#include "container/vector.h"

#include "graphics/vertex_format.h"
#include "graphics/sub_mesh.h"
#include "graphics/shader.h"
#include "graphics/pipeline.h"

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

		void init(Backbuffer* backbuffer);
		void cleanUp();

		void render(const Camera& camera, float deltaTime);

		const Vector<RenderObject>::Iterator& createRenderObject();

	private:
		void loadTextures();
		void createQuadMesh();
		void createSkybox();
		void addRenderObjects();
		void createPostProcessResources();

		void renderSkybox(const Camera& camera);
		void renderObjects(const Camera& camera);
		void renderParticles(const Camera& camera, float deltaTime);
		void renderPostProcess();

		void aggregateSubMeshes(Vector<SubMesh*>& list);
		void sortRenderListByMaterialHash(int lo, int hi);

		Backbuffer* m_backbuffer;

		SubMesh m_quadMesh;
		SubMesh m_skyboxMesh;

		RenderTarget* m_gBuffer;

		ShaderParameters m_pushConstants;

		Vector<RenderObject> m_renderObjects;
		Vector<SubMesh*> m_renderList;

		Mesh* m_blockMesh;

		Material* m_skyboxMaterial;

//		GraphicsPipeline m_postProcessPipeline;
//		VkDescriptorSet m_postProcessDescriptorSet;

//		DescriptorPoolDynamic m_descriptorPool;

//		GPUParticles m_gpuParticles;

		int m_frameCount;
	};
}

#endif // RENDERER_H_
