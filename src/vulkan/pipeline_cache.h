#ifndef PIPELINE_CACHE_H_
#define PIPELINE_CACHE_H_

#include "third_party/volk.h"

#include "container/hash_map.h"

namespace llt
{
	class GraphicsPipelineDefinition;
	class ComputePipelineDefinition;

	class RenderInfo;
	class ShaderEffect;

	struct PipelineData
	{
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	class PipelineCache
	{
	public:
		PipelineCache();
		~PipelineCache();

		void init();
		void dispose();

		PipelineData fetchGraphicsPipeline(const GraphicsPipelineDefinition &definition, const RenderInfo &renderInfo);
		PipelineData fetchComputePipeline(const ComputePipelineDefinition &definition);

		VkPipelineLayout fetchPipelineLayout(const ShaderEffect *shader);

	private:
		HashMap<uint64_t, VkPipeline> m_pipelines;
		HashMap<uint64_t, VkPipelineLayout> m_layouts;
	};
}

#endif // PIPELINE_CACHE_H_
