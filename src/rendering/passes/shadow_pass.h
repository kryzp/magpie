#ifndef SHADOW_PASS_H_
#define SHADOW_PASS_H_

#include "container/vector.h"

namespace llt
{
	class CommandBuffer;
	class SubMesh;

	class ShadowPass
	{
	public:
		ShadowPass() = default;
		~ShadowPass() = default;

		void init();
		void dispose();

		void render(CommandBuffer &cmd, const Vector<SubMesh *> &renderList);
	};

	extern ShadowPass g_shadowPass;
}

#endif // SHADOW_PASS_H_
