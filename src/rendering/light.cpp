#include "light.h"

using namespace mgp;

Light::Light()
	: m_type(TYPE_POINT)
	, m_colour()
	, m_shadowsEnabled(false)
	, m_intensity(0.0f)
	, m_falloff(0.0f)
	, m_direction({ 1.0f, 0.0f, 0.0f })
	, m_position({ 0.0f, 0.0f, 0.0f })
{
}

Light::~Light()
{
}

Light::LightType Light::getType() const
{
	return m_type;
}

void Light::setType(Light::LightType type)
{
	m_type = type;
}

const Colour &Light::getColour() const
{
	return m_colour;
}

void Light::setColour(const Colour &colour)
{
	m_colour = colour;
}

bool Light::isShadowCaster() const
{
	return m_shadowsEnabled;
}

void Light::toggleShadows(bool enabled)
{
	m_shadowsEnabled = enabled;
}

const ShadowMapManager::AtlasRegion &Light::getShadowAtlasRegion() const
{
	return m_shadowAtlasRegion;
}

void Light::setShadowAtlasRegion(const ShadowMapManager::AtlasRegion &region)
{
	m_shadowAtlasRegion = region;
}

float Light::getFalloff() const
{
	return m_falloff;
}

void Light::setFalloff(float falloff)
{
	m_falloff = falloff;
}

float Light::getIntensity() const
{
	return m_intensity;
}

void Light::setIntensity(float intensity)
{
	m_intensity = intensity;
}

const glm::vec3 &Light::getDirection() const
{
	return m_direction;
}

void Light::setDirection(const glm::vec3 &direction)
{
	m_direction = direction;
}

const glm::vec3 &Light::getPosition() const
{
	return m_position;
}

void Light::setPosition(const glm::vec3 &position)
{
	m_position = position;
}
