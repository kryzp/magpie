#include "render/light.h"

#include <cmath>

float rnd::Light::HeuristicRadius(float epsilon_intensity) const
{
	float maximum = colour.MaxComponent();
	return std::sqrt((intensity * maximum) / (falloff * epsilon_intensity));
}
