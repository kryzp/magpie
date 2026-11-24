#include "core/ease.h"
#include "core/maths.h"

#include <cmath>

static const float SwingScale = 1.5f;
static const float ElasticBounces = 7.f * maths::PIf * -1.f;

float ease::Swing(float a)
{
	if (a <= 0.5f) {
		a *= 2.f;
		return a * a * ((SwingScale + 1.f) * a - SwingScale) / 2.f;
	}

	a -= 1.f;
	a *= 2.f;

	return a * a * ((SwingScale + 1.f) * a + SwingScale) / 2.f + 1.f;
}

float ease::ElasticIn(float a)
{
	if (a >= 0.99f)
		return 1.f;

	return std::pow(2.f, 10.f * (a - 1.f)) * std::sin(a * ElasticBounces);
}

float ease::ElasticOut(float a)
{
	if (a == 0.f)
		return 0.f;

	a = 1.f - a;

	return 1.f - std::pow(2.f, 10.f * (a - 1.f)) * std::sin(a * ElasticBounces);
}
