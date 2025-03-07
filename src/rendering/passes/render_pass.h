#ifndef RENDER_PASS_H_
#define RENDER_PASS_H_

namespace llt
{
	class DescriptorPoolDynamic;

	class RenderPass
	{
	public:
		RenderPass() = default;
		virtual ~RenderPass() = default;

		virtual void init(DescriptorPoolDynamic& pool) = 0;
		virtual void cleanUp() = 0;
	};
}

#endif // RENDER_PASS_H_
