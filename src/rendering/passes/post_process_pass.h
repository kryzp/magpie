#ifndef POST_PROCESS_PASS_H_
#define POST_PROCESS_PASS_H_

#include "render_pass.h"

#include "vulkan/pipeline.h"

namespace llt
{
	class CommandBuffer;
	class GenericRenderTarget;

	class PostProcessPass : public RenderPass
	{
	public:
		PostProcessPass() = default;
		~PostProcessPass() override = default;

		void init(DescriptorPoolDynamic& pool) override;
		void cleanUp() override;

		void render(CommandBuffer &buffer, GenericRenderTarget *input);

	private:
		Pipeline m_hdrPipeline;
		VkDescriptorSet m_hdrSet;
	};
}

#endif // POST_PROCESS_PASS_H_
