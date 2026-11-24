#include "core/colour.h"
#include "core/maths.h"
#include "core/vec3.h"
#include "core/vec4.h"

#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Colour::Colour(float r_, float g_, float b_, float a_)
	: r(r_)
	, g(g_)
	, b(b_)
	, a(a_)
{
}

Colour Colour::Lerp(const Colour &to, float t) const
{
	return Colour(
		r + (to.r - r) * t,
		g + (to.g - g) * t,
		b + (to.b - b) * t,
		a + (to.a - a) * t
	);
}

Colour Colour::Clamped() const
{
	return Colour(maths::Saturate(r), maths::Saturate(g), maths::Saturate(b), maths::Saturate(a));
}

Colour Colour::WithAlpha(float alpha) const
{
	return Colour(r, g, b, alpha);
}

Colour Colour::Premultiplied() const
{
	return Colour(r * a, g * a, b * a, a);
}

Vec4 Colour::ToVec4() const
{
	return Vec4(r, g, b, a);
}

Vec3 Colour::ToVec3() const
{
	return Vec3(r, g, b);
}

u32 Colour::ToPacked8888() const
{
	Colour c = Clamped();

	return
		((u32)(c.r * 255.f) << 24) |
		((u32)(c.g * 255.f) << 16) |
		((u32)(c.b * 255.f) <<  8) |
		((u32)(c.a * 255.f) <<  0);
}

Colour Colour::FromPacked8888(u32 rgba)
{
	return Colour(
		((rgba >> 24) & 0xFF) / 255.f,
		((rgba >> 16) & 0xFF) / 255.f,
		((rgba >> 8) & 0xFF) / 255.f,
		((rgba) & 0xFF) / 255.f
	);
}

String Colour::ToHex() const
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%08x", ToPacked8888());
	return buf;
}

Colour Colour::FromHex(const char *hex)
{
	if (hex[0] == '#')
		hex++;

	unsigned length = strlen(hex);
	unsigned value = strtoul(hex, nullptr, 16);

	if (length <= 6)
		value = (value << 8) | 0xFF;

	return FromPacked8888(value);
}

Colour Colour::FromHSV(float hue_degrees, float saturation, float value, float alpha)
{
	float h = std::fmod(hue_degrees / 60.f + 6.f, 6.f);
	int i = (int)std::floor(h);
	float f = h - (float)i;
	float p = value * (1.f - saturation);
	float q = value * (1.f - saturation * f);
	float t = value * (1.f - saturation * (1.f - f));

	switch (i) {
		case 0:   return Colour(value, t, p, alpha);
		case 1:   return Colour(q, value, p, alpha);
		case 2:   return Colour(p, value, t, alpha);
		case 3:   return Colour(p, q, value, alpha);
		case 4:   return Colour(t, p, value, alpha);
		default:  return Colour(value, p, q, alpha);
	}
}
