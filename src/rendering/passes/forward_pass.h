#ifndef FORWARD_PASS_H_
#define FORWARD_PASS_H_

#include "container/vector.h"

namespace llt
{
	class CommandBuffer;
	class Camera;
	class SubMesh;
	class GPUBuffer;

	class ForwardPass
	{
	public:
		ForwardPass() = default;
		~ForwardPass() = default;

		void init();
		void dispose();

		void render(CommandBuffer &cmd, const Camera &camera, const Vector<SubMesh *> &renderList);

	private:
		GPUBuffer *m_frameConstantsBuffer;
		GPUBuffer *m_transformationBuffer;
	};

	extern ForwardPass g_forwardPass;
}

#endif // FORWARD_PASS_H_
