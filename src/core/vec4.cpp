#include "vec4.h"
#include "vec3.h"
#include "maths.h"

Vec4::Vec4()
	: x(0.f)
	, y(0.f)
	, z(0.f)
	, w(0.f)
{
}

Vec4::Vec4(float x, float y, float z, float w)
	: x(x)
	, y(y)
	, z(z)
	, w(w)
{
}

Vec4::Vec4(const Vec3 &xyz, float w)
	: x(xyz.x)
	, y(xyz.y)
	, z(xyz.z)
	, w(w)
{
}

Vec4::Vec4(float s)
	: x(s)
	, y(s)
	, z(s)
	, w(s)
{
}

Vec3 Vec4::XYZ() const
{
	return Vec3(x, y, z);
}

float Vec4::Dot(const Vec4 &other) const
{
	return (
		x * other.x +
		y * other.y +
		z * other.z +
		w * other.w
	);
}

float Vec4::LengthSqr() const
{
	return Dot(*this);
}

float Vec4::Length() const
{
	return std::sqrt(LengthSqr());
}

Vec4 Vec4::Lerp(const Vec4 &to, float t) const
{
	return Vec4(
		maths::Lerp(x, to.x, t),
		maths::Lerp(y, to.y, t),
		maths::Lerp(z, to.z, t),
		maths::Lerp(w, to.w, t)
	);
}

Vec4 Vec4::FrustumNormalizePlane() const
{
	float length = std::sqrt(x*x + y*y + z*z);
	return *this * (1.f / length);
}
