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

struct Colour {
	u8 r;
	u8 g;
	u8 b;
	u8 a;

	// TODO
};
