#ifndef LIGHT_H_
#define LIGHT_H_

#include <glm/vec4.hpp>

namespace llt
{
	enum LightType
	{
		LIGHT_TYPE_SUN,
		LIGHT_TYPE_SPOTLIGHT,
		LIGHT_TYPE_POINT
	};

	struct Light
	{
		glm::vec3 position;
		float radius;
		glm::vec3 colour;
		float attenuation;
		glm::vec3 direction;
		float type;
	};
}

#endif // LIGHT_H_
