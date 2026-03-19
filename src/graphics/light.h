#pragma once

#include "math/vec3.h"
#include "math/colour.h"
#include "math/calc.h"

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

		float get_heuristic_radius(float epsilon_intensity) const
		{
			float maximum = colour.max_value();
			return CalcF::sqrt((intensity * maximum) / (falloff * epsilon_intensity));
		}
	};
}
