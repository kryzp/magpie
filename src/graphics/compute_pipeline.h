#ifndef COMPUTE_PIPELINE_H_
#define COMPUTE_PIPELINE_H_

#include "pipeline.h"
#include "queue.h"

namespace llt
{
	class ComputePipeline : public Pipeline
	{
	public:
		ComputePipeline();
		~ComputePipeline() override;

		void dispatch(int gcX, int gcY, int gcZ);

		VkPipeline getPipeline() override;

		void bindShader(const ShaderProgram* shader) override;

	private:
		VkPipelineShaderStageCreateInfo m_computeShaderStageInfo;
	};
}

#endif // COMPUTE_PIPELINE_H_
