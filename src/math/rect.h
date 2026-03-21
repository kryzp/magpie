#pragma once

#include "core/types.h"

struct RectI {
	int x;
	int y;
	int w;
	int h;

	RectI()
	{
	}

	RectI(int x, int y, int w, int h)
		: x(x), y(y), w(w), h(h)
	{
	}
};

struct Rect {
	float x;
	float y;
	float w;
	float h;

	Rect()
	{
	}

	Rect(float x, float y, float w, float h)
		: x(x), y(y), w(w), h(h)
	{
	}
};
