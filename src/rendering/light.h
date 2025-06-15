#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "math/colour.h"

namespace mgp
{
	constexpr static unsigned MAX_POINT_LIGHTS = 16;

	struct GPU_PointLight
	{
		glm::vec4 position; // [x,y,z]: position
		glm::vec4 colour; // [x,y,z]: colour, [w]: intensity
		glm::vec4 attenuation; // [x]*dist^2 + [y]*dist + [z], [w]: has shadows? 0/1
	};

	using LightId = unsigned;

	class Light
	{
		friend class LightHandle;

	public:
		enum LightType
		{
			TYPE_POINT
		};

		Light() = default;
		~Light() = default;

		LightType getType() const { return m_type; }
		void setType(LightType type) { m_type = type; }

		const Colour &getColour() const { return m_colour; }
		void setColour(const Colour &colour) { m_colour = colour; }

		float getFalloff() const { return m_falloff; }
		void setFalloff(float falloff) { m_falloff = falloff; }

		float getIntensity() const { return m_intensity; }
		void setIntensity(float intensity) { m_intensity = intensity; }

		const glm::vec3 &getDirection() const { return m_direction; }
		void setDirection(const glm::vec3 &direction) { m_direction = direction; }

		const glm::vec3 &getPosition() const { return m_position; }
		void setPosition(const glm::vec3 &position) { m_position = position; }

	private:
		LightType m_type;

		Colour m_colour;

		float m_intensity;
		float m_falloff;

		glm::vec3 m_direction;
		glm::vec3 m_position;
	};
}
