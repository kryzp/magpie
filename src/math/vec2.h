#pragma once

struct Vec2
{
	union {
		struct {
			float x;
			float y;
		};
		float v[2];
	};

	Vec2();
	Vec2(float s);
	Vec2(float x, float y);

	static const Vec2 &unit();
	static const Vec2 &zero();
	static const Vec2 &one();
	static const Vec2 &left();
	static const Vec2 &right();
	static const Vec2 &up();
	static const Vec2 &down();

	static float dot(const Vec2 &a, const Vec2 &b);
	static float cross(const Vec2 &a, const Vec2 &b);
	static Vec2 from_angle(float radius, float angle);
	static Vec2 lerp(const Vec2 &from, const Vec2 &to, float amount);
	static Vec2 approach(const Vec2 &from, const Vec2 &to, float amount);

	float angle() const;

	float length() const;
	float length_squared() const;

	float min_value() const;
	float max_value() const;

	Vec2 abs() const;
	Vec2 normalized() const;
	Vec2 perpendicular() const;

	bool operator == (const Vec2 &other) const;
	bool operator != (const Vec2 &other) const;

	Vec2 operator + (const Vec2 &other) const;
	Vec2 operator - (const Vec2 &other) const;
	Vec2 operator * (const Vec2 &other) const;
	Vec2 operator / (const Vec2 &other) const;

	Vec2 operator - () const;

	Vec2 &operator += (const Vec2 &other);
	Vec2 &operator -= (const Vec2 &other);
	Vec2 &operator *= (const Vec2 &other);
	Vec2 &operator /= (const Vec2 &other);
};
