#include "light.h"

using namespace mgp;

PointLight::PointLight()
	: m_position(0.0f, 0.0f, 0.0f)
	, m_colour(0.0f, 0.0f, 0.0f)
{
}

ShaderPointLight PointLight::getShaderLight() const
{
	ShaderPointLight light;
	light.position = { m_position.x, m_position.y, m_position.z, 0.0f };
	light.colour = { m_colour.x, m_colour.y, m_colour.z, 0.0f };

	return light;
}

void PointLight::setPosition(const glm::vec3 &position)
{
	m_position = position;
}

void PointLight::setColour(const glm::vec3 &colour)
{
	m_colour = colour;
}
