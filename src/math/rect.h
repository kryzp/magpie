#pragma once

#include "core/types.h"

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
