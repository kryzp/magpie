#ifndef RENDERER_H_
#define RENDERER_H_

#include "container/vector.h"

#include "graphics/vertex_descriptor.h"
#include "graphics/sub_mesh.h"
#include "graphics/shader.h"
#include "graphics/graphics_pipeline.h"
#include "graphics/compute_pipeline.h"

#include "entity.h"
#include "gpu_particles.h"

namespace llt
{
	class Backbuffer;
	class ShaderProgram;
	class ShaderBuffer;
	class Camera;

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void init(Backbuffer* backbuffer);

		void render(const Camera& camera, float deltaTime, float elapsedTime);

	private:
		void loadTextures();
		void createQuadMesh();
		void createSkybox();
//		void createEntities();

		void renderSkybox(const Camera& camera, float deltaTime, float elapsedTime);
		void renderEntities(const Camera& camera, float deltaTime, float elapsedTime);
//		void renderParticles(const Camera& camera, float deltaTime, float elapsedTime);

		Backbuffer* m_backbuffer;

		SubMesh m_quadMesh;
		SubMesh m_skyboxMesh;

//		RenderTarget* m_target;

		ShaderParameters m_pushConstants;

//		GPUParticles m_gpuParticles;

		Vector<Entity> m_renderEntities;

		glm::mat4 m_prevViewMatrix;

		Material* m_skyboxMaterial;

//		GraphicsPipeline m_postProcessPipeline;

		int m_frameCount;
	};
}

#endif // RENDERER_H_
