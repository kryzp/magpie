/*
#ifndef GPU_PARTICLES_H_
#define GPU_PARTICLES_H_

#include "rendering/vertex_descriptor.h"
#include "rendering/sub_mesh.h"
#include "rendering/shader.h"
#include "rendering/graphics_pipeline.h"
#include "rendering/compute_pipeline.h"

#include "camera.h"

namespace llt
{
	class DynamicShaderBuffer;

	class GPUParticles
	{
		constexpr static int PARTICLE_COUNT = 1;

	public:
		GPUParticles();
		~GPUParticles();

		void init(const DynamicShaderBuffer *shaderParams);

		void dispatchCompute(const Camera &camera);
		void render();

	private:
		DynamicShaderBuffer *m_particleBuffer;

		ShaderProgram *m_computeProgram;
		ShaderParameters m_computeParams;
		DynamicShaderBuffer *m_computeParamsBuffer;

		SubMesh m_particleMesh;
		VertexDescriptor m_particleVertexFormat;

		GraphicsPipeline m_particleGraphicsPipeline;
		ComputePipeline m_particleComputePipeline;

		glm::mat4 m_prevViewMatrix;
	};
}

#endif // GPU_PARTICLES_H_
*/
