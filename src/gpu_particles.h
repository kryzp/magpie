#ifndef GPU_PARTICLES_H_
#define GPU_PARTICLES_H_

#include "graphics/vertex_descriptor.h"
#include "graphics/sub_mesh.h"
#include "graphics/shader.h"
#include "graphics/graphics_pipeline.h"
#include "graphics/compute_pipeline.h"

#include "camera.h"

namespace llt
{
	class ShaderBuffer;

	class GPUParticles
	{
		constexpr static int PARTICLE_COUNT = 1;

	public:
		GPUParticles();
		~GPUParticles();

		void init(const ShaderBuffer* shaderParams);

		void dispatchCompute(const Camera& camera);
		void render();

	private:
		ShaderBuffer* m_particleBuffer;

		ShaderProgram* m_computeProgram;
		ShaderParameters m_computeParams;
		ShaderBuffer* m_computeParamsBuffer;

		SubMesh m_particleMesh;
		VertexDescriptor m_particleVertexFormat;

		GraphicsPipeline m_particleGraphicsPipeline;
		ComputePipeline m_particleComputePipeline;

		glm::mat4 m_prevViewMatrix;
	};
}

#endif // GPU_PARTICLES_H_
