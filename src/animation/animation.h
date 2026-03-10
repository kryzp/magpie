#pragma once

#include "math/vec3.h"
#include "math/quat.h"
#include "math/mat4.h"

#include "container/vector.h"

namespace anim
{
	struct AnimationNode {
		const char *name;
		Mat4 transform;
		Vector<AnimationNode> children;
	};

	class Animation {
	public:
	private:
	};
}
