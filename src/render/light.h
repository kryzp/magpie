#ifndef RENDER_LIGHT_H
#define RENDER_LIGHT_H

#include "core/vec3.h"

namespace rnd {

enum LightType {
	LightType_Point,
	LightType_MaxEnum
};

struct Light {
	LightType type = LightType_Point;

	Vec3 position;
	Vec3 direction;

	Vec3 colour;
	float intensity = 0.f;
	float falloff = 0.f;

	bool casts_shadows = false;
	float shadow_near = 0.f;
	float shadow_far = 0.f;

	[[nodiscard]] float HeuristicRadius(float epsilon_intensity) const;
};

}

#endif // RENDER_LIGHT_H
