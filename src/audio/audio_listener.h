#pragma once

#include "math/vec3.h"

namespace audio
{
	struct AudioListener {
		Vec3 eye;
		Vec3 forward;
		Vec3 up;
	};
}
