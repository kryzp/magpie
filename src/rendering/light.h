#pragma once

#include <glm/glm.hpp>

namespace mgp
{
	constexpr static unsigned MAX_POINT_LIGHTS = 16;

	struct ShaderPointLight
	{
		glm::vec4 position; // [x,y,z]: position, [w]: unused
		glm::vec4 colour; // [x,y,z]: colour, [w]: unused
	};

	class PointLight
	{
	public:
		PointLight();
		~PointLight() = default;

		ShaderPointLight getShaderLight() const;

		void setPosition(const glm::vec3 &position);
		void setColour(const glm::vec3 &colour);

	private:
		glm::vec3 m_position;
		glm::vec3 m_colour;
	};
}
