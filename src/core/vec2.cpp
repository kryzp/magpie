#include "vec2.h"
#include "maths.h"

Vec2::Vec2()
	: x(0.f)
	, y(0.f)
{
}

Vec2::Vec2(float x, float y)
	: x(x)
	, y(y)
{
}

Vec2::Vec2(float s)
	: x(s)
	, y(s)
{
}

Vec2 Vec2::FromAngleRad(float radians, float length)
{
	return Vec2(
		std::cos(radians) * length,
		std::sin(radians) * length
	);
}

float Vec2::Dot(const Vec2 &other) const
{
	return (
		x * other.x +
		y * other.y
	);
}

float Vec2::Cross(const Vec2 &other) const
{
	return (x * other.y) - (y * other.x);
}

float Vec2::LengthSqr() const
{
	return Dot(*this);
}

float Vec2::Length() const
{
	return std::sqrt(LengthSqr());
}

float Vec2::Distance(const Vec2 &other) const
{
	return (other - *this).Length();
}

float Vec2::DistanceSqr(const Vec2 &other) const
{
	return (other - *this).LengthSqr();
}

Vec2 Vec2::Normalized() const
{
	if (maths::WithinEpsilon(LengthSqr()))
		return Vec2(0.f);

	return *this * (1.f / Length());
}

Vec2 Vec2::WithLength(float length) const
{
	return Normalized() * length;
}

Vec2 Vec2::Limited(float hi) const
{
	float length_sqr = LengthSqr();

	if (length_sqr > hi * hi)
		return *this * (hi / std::sqrt(length_sqr));

	return *this;
}

Vec2 Vec2::Clamped(float lo, float hi) const
{
	float length = Length();

	if (length <= 0.f)
		return *this;

	float clamped = maths::Clamp(length, lo, hi);

	return *this * (clamped / length);
}

Vec2 Vec2::Lerp(const Vec2 &to, float t) const
{
	return Vec2(
		maths::Lerp(x, to.x, t),
		maths::Lerp(y, to.y, t)
	);
}

Vec2 Vec2::RotatedRad(float radians) const
{
	float c = std::cos(radians);
	float s = std::sin(radians);

	return Vec2(
		x * c - y * s,
		x * s + y * c
	);
}

float Vec2::AngleRad() const
{
	return std::atan2(y, x);
}

Vec2 Vec2::Limited2(float max_length_sqr) const
{
	float len2 = LengthSqr();

	if (len2 > max_length_sqr)
		return *this * std::sqrt(max_length_sqr / len2);

	return *this;
}

bool Vec2::IsZero() const
{
	return x == 0.f && y == 0.f;
}

bool Vec2::IsZero(float margin) const
{
	return LengthSqr() < margin;
}
