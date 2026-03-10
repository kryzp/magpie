#pragma once

#include "core/types.h"

struct DisplayColour {
	float r;
	float g;
	float b;
	float a;

	DisplayColour()
		: r(0.f), g(0.f), b(0.f), a(0.f)
	{
	}

	DisplayColour(float r, float g, float b, float a)
		: r(r), g(g), b(b), a(a)
	{
	}
};

struct Colour
{
	union
	{
		struct
		{
			u8 r;
			u8 g;
			u8 b;
			u8 a;
		};

		u8 data[4];
	};

	Colour();
	Colour(u8 r, u8 g, u8 b, u8 a = 255);
	Colour(u32 packed);

	static const Colour &empty();
	static const Colour &black();
	static const Colour &white();
	static const Colour &red();
	static const Colour &green();
	static const Colour &blue();
	static const Colour &yellow();
	static const Colour &magenta();
	static const Colour &cyan();

	static Colour from_hsv(float hue, float sat, float val, u8 alpha = 255);
	static Colour lerp(const Colour &from, const Colour &to, float amount);

	u32 packed() const;

	Colour premultiplied() const;

	void export_to_u8(u8 *colours) const;
	void export_to_float(float *colours) const;

	bool operator == (const Colour& other) const;
	bool operator != (const Colour& other) const;

	Colour operator - () const;
	Colour operator * (float factor) const;
	Colour operator / (float factor) const;

	Colour& operator *= (float factor);
	Colour& operator /= (float factor);
};
