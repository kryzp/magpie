#ifndef CORE_COLOUR_H
#define CORE_COLOUR_H

#include "types.h"
#include "container/string.h"

struct Vec4;
struct Vec3;

struct Colour {
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 0.f;

	Colour() = default;

	Colour(float r_, float g_, float b_, float a_ = 1.f);

	static Colour FromHex(const char *hex); // "RRGGBB" or "RRGGBBAA". can have a leading #
	static Colour FromHSV(float hue_degrees, float saturation, float value, float alpha = 1.f);

	Colour Lerp(const Colour &to, float t) const;
	Colour Clamped() const;

	Colour WithAlpha(float alpha) const;

	Colour Premultiplied() const;

	Vec4 ToVec4() const;
	Vec3 ToVec3() const;

	// 0xRRGGBBAA
	[[nodiscard]] u32 ToPacked8888() const;
	static Colour FromPacked8888(u32 rgba);

	// "rrggbbaa"
	String ToHex() const;

	static Colour White()
	{
		return Colour(1.f, 1.f, 1.f, 1.f);
	}

	static Colour Black()
	{
		return Colour(0.f, 0.f, 0.f, 1.f);
	}

	static Colour Clear()
	{
		return Colour(0.f, 0.f, 0.f, 0.f);
	}

	static Colour Red()
	{
		return Colour(1.f, 0.f, 0.f, 1.f);
	}

	static Colour Green()
	{
		return Colour(0.f, 1.f, 0.f, 1.f);
	}

	static Colour Blue()
	{
		return Colour(0.f, 0.f, 1.f, 1.f);
	}

	static Colour Yellow()
	{
		return Colour(1.f, 1.f, 0.f, 1.f);
	}

	static Colour Cyan()
	{
		return Colour(0.f, 1.f, 1.f, 1.f);
	}

	static Colour Magenta()
	{
		return Colour(1.f, 0.f, 1.f, 1.f);
	}

	static Colour Gray()
	{
		return Colour(0.5f, 0.5f, 0.5f, 1.f);
	}

	static Colour LightGray()
	{
		return Colour(0.75f, 0.75f, 0.75f, 1.f);
	}

	static Colour DarkGray()
	{
		return Colour(0.25f, 0.25f, 0.25f, 1.f);
	}

	static Colour Orange()
	{
		return Colour(1.f, 0.647f, 0.f, 1.f);
	}

	Colour operator*(const Colour &other) const
	{
		return Colour(r * other.r, g * other.g, b * other.b, a * other.a);
	}

	Colour operator*(float f) const
	{
		return Colour(r * f, g * f, b * f, a * f);
	}

	Colour operator+(const Colour &other) const
	{
		return Colour(r + other.r, g + other.g, b + other.b, a + other.a);
	}

	bool operator==(const Colour &other) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	bool operator!=(const Colour &other) const
	{
		return !(*this == other);
	}
};

#endif // CORE_COLOUR_H
