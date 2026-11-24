#include "rect.h"
#include "vec2.h"

Rect::Rect()
	: x(0.f)
	, y(0.f)
	, width(0.f)
	, height(0.f)
{
}

Rect::Rect(float x, float y, float w, float h)
	: x(x)
	, y(y)
	, width(w)
	, height(h)
{
}

bool Rect::Contains(const Vec2 &point) const
{
	return Contains(point.x, point.y);
}

bool Rect::Contains(float px, float py) const
{
	return
		px >= x && px <= x + width &&
		py >= y && py <= y + height;
}

bool Rect::Overlaps(const Rect &other) const
{
	return
		x < other.x + other.width && x + width > other.x &&
		y < other.y + other.height && y + height > other.y;
}

Vec2 Rect::Centre() const
{
	return Vec2(x + width * 0.5f, y + height * 0.5f);
}