#ifndef FORWARD_PASS_H_
#define FORWARD_PASS_H_

#include "container/vector.h"

#include "render_pass.h"

namespace llt
{
	class CommandBuffer;
	class Camera;
	class SubMesh;

	class ForwardPass : public RenderPass
	{
	public:
		ForwardPass() = default;
		~ForwardPass() override = default;

		void init(DescriptorPoolDynamic& pool) override;
		void cleanUp() override;

		void render(CommandBuffer &cmd, const Camera &camera, const Vector<SubMesh *> &renderList);
	};
}

#endif // FORWARD_PASS_H_
