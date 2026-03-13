#pragma once

#include "math/vec3.h"
#include "math/colour.h"

namespace gfx
{
	struct Light {
		enum LightType {
			TYPE_POINT,
			TYPE_MAX_ENUM
		};

		LightType type;

		Vec3 position;
		Vec3 direction;

		Vec3 colour;
		float intensity;
		float falloff;

		bool casts_shadows;
		float shadow_near;
		float shadow_far;
	};
}
