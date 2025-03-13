#ifndef FORWARD_PASS_H_
#define FORWARD_PASS_H_

#include "container/vector.h"

namespace llt
{
	class CommandBuffer;
	class Camera;
	class SubMesh;

	class ForwardPass
	{
	public:
		ForwardPass() = default;
		~ForwardPass() = default;

		void init();
		void cleanUp();

		void render(CommandBuffer &cmd, const Camera &camera, const Vector<SubMesh *> &renderList);
	};

	extern ForwardPass g_forwardPass;
}

#endif // FORWARD_PASS_H_
