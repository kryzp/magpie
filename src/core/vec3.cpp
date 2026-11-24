#include "vec3.h"
#include "vec2.h"
#include "maths.h"

Vec3::Vec3()
	: x(0.f)
	, y(0.f)
	, z(0.f)
{
}

Vec3::Vec3(float x, float y, float z)
	: x(x)
	, y(y)
	, z(z)
{
}

Vec3::Vec3(const Vec2 &xy, float z)
	: x(xy.x)
	, y(xy.y)
	, z(z)
{
}

Vec3::Vec3(float s)
	: x(s)
	, y(s)
	, z(s)
{
}

Vec3 Vec3::SphericalToCartesian(float radius, float azimuth, float elevation)
{
	return Vec3(
		radius * std::cos(elevation) * std::cos(azimuth),
		radius * std::cos(elevation) * std::sin(azimuth),
		radius * std::sin(elevation)
	);
}

Vec3 Vec3::RandomDirection()
{
	float z = maths::Random(-1.f, 1.f);
	float theta = maths::Random(0.f, maths::PI2f);
	float r = std::sqrt(1.f - z * z);

	return Vec3(r * std::cos(theta), r * std::sin(theta), z);
}

Vec2 Vec3::XY() const
{
	return Vec2(x, y);
}

float Vec3::Dot(const Vec3 &other) const
{
	return (
		x * other.x +
		y * other.y +
		z * other.z
	);
}

Vec3 Vec3::Cross(const Vec3 &other) const
{
	return Vec3(
		(y * other.z) - (z * other.y),
		(z * other.x) - (x * other.z),
		(x * other.y) - (y * other.x)
	);
}

float Vec3::LengthSqr() const
{
	return Dot(*this);
}

float Vec3::Length() const
{
	return std::sqrt(LengthSqr());
}

float Vec3::Distance(const Vec3 &other) const
{
	return (other - *this).Length();
}

float Vec3::DistanceSqr(const Vec3 &other) const
{
	return (other - *this).LengthSqr();
}

Vec3 Vec3::Normalized() const
{
	if (maths::WithinEpsilon(LengthSqr()))
		return Vec3(0.f);

	return *this * (1.f / Length());
}

float Vec3::MaxComponent() const
{
	return maths::Max(x, maths::Max(y, z));
}

float Vec3::MinComponent() const
{
	return maths::Min(x, maths::Min(y, z));
}

Vec3 Vec3::MinOf(const Vec3 &other) const
{
	return Vec3(
		maths::Min(x, other.x),
		maths::Min(y, other.y),
		maths::Min(z, other.z)
	);
}

Vec3 Vec3::MaxOf(const Vec3 &other) const
{
	return Vec3(
		maths::Max(x, other.x),
		maths::Max(y, other.y),
		maths::Max(z, other.z)
	);
}

Vec3 Vec3::Lerp(const Vec3 &to, float amount) const
{
	return Vec3(
		maths::Lerp(x, to.x, amount),
		maths::Lerp(y, to.y, amount),
		maths::Lerp(z, to.z, amount)
	);
}

Vec3 Vec3::Approach(const Vec3 &to, float amount) const
{
	return Vec3(
		maths::Approach(x, to.x, amount),
		maths::Approach(y, to.y, amount),
		maths::Approach(z, to.z, amount)
	);
}

Vec3 Vec3::Reflect(const Vec3 &normal) const
{
	// reflected = v - 2(v.n)n
	return *this - normal * (2.f * Dot(normal));
}

Vec3 Vec3::Refract(const Vec3 &normal, double eta21) const
{
	double cost = maths::Min((double)-Dot(normal), 1.0);
	Vec3 out_perp = (*this + normal * (float)cost) * (float)eta21; // eta . (v + cost . n)
	Vec3 out_para = normal * -std::sqrt(maths::Abs(1.f - out_perp.LengthSqr()));
	return out_perp + out_para;
}

Vec3 Vec3::Limit(float min_length, float max_length) const
{
	float length = Length();
	Vec3 normal = Normalized();

	return normal * maths::Clamp(length, min_length, max_length);
}

bool Vec3::IsZero() const
{
	return x == 0.f && y == 0.f && z == 0.f;
}

bool Vec3::IsZero(float margin) const
{
	return LengthSqr() < margin;
}

Vec3 Vec3::Limited(float max_length) const
{
	float len2 = LengthSqr();

	if (len2 > max_length * max_length)
		return *this * std::sqrt((max_length * max_length) / len2);

	return *this;
}
