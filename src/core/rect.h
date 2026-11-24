#ifndef CORE_RECT_H
#define CORE_RECT_H

struct Vec2;

struct Rect {
	float x;
	float y;
	float width;
	float height;

	Rect();
	Rect(float x, float y, float w, float h);

	[[nodiscard]] bool Contains(const Vec2 &point) const;
	[[nodiscard]] bool Contains(float px, float py) const;
	[[nodiscard]] bool Overlaps(const Rect &other) const;
	[[nodiscard]] Vec2 Centre() const;
};

#endif // CORE_RECT_H
