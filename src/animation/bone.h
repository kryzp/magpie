#pragma once

#include "math/vec3.h"
#include "math/quat.h"

namespace anim
{
	struct KeyPosition {
		float timestamp;
		Vec3 position;
	};

	struct KeyRotation {
		float timestamp;
		Quat rotation;
	};

	struct KeyScale {
		float timestamp;
		Vec3 scale;
	};

	class Bone {
	public:
	private:
	};
}
