#include "quat.h"
#include "vec3.h"
#include "vec4.h"
#include "maths.h"

Quat::Quat()
	: x(0.f)
	, y(0.f)
	, z(0.f)
	, w(1.f)
{
}

Quat::Quat(float x, float y, float z, float w)
	: x(x)
	, y(y)
	, z(z)
	, w(w)
{
}

float Quat::Dot(const Quat &other) const
{
	return (
		x * other.x +
		y * other.y +
		z * other.z +
		w * other.w
	);
}

float Quat::LengthSqr() const
{
	return Dot(*this);
}

float Quat::Length() const
{
	return std::sqrt(LengthSqr());
}

Quat Quat::FromAxis(float angle, const Vec3 &axis)
{
	Quat q;

	q.w = std::cos(angle * .5f);
	q.x = std::sin(angle * .5f) * axis.x;
	q.y = std::sin(angle * .5f) * axis.y;
	q.z = std::sin(angle * .5f) * axis.z;

	return q;
}

Quat Quat::FromEuler(const Vec3 &euler)
{
	// Pitch
	float sp = std::sin(euler.x * .5f);
	float cp = std::cos(euler.x * .5f);

	// Yaw
	float sy = std::sin(euler.y * .5f);
	float cy = std::cos(euler.y * .5f);

	// Roll
	float sr = std::sin(euler.z * .5f);
	float cr = std::cos(euler.z * .5f);

	return Quat(
		(sr * cp * cy) - (cr * sp * sy),
		(cr * sp * cy) + (sr * cp * sy),
		(cr * cp * sy) - (sr * sp * cy),
		(cr * cp * cy) + (sr * sp * sy)
	);
}

Quat Quat::LookAt(const Vec3 &eye, const Vec3 &centre)
{
	Vec3 fwd = Vec3(0.f, 1.f, 0.f);

	Vec3 dir = (centre - eye).Normalized();

	float d = fwd.Dot(dir);

	if (d > 0.9999f)
		return Quat(0.f, 0.f, 0.f, 1.f);

	if (d < -0.9999f)
		return Quat(0.f, 0.f, 1.f, 0.f);

	Vec3 axis = fwd.Cross(dir);

	float s = std::sqrt((1.f + d) * 2.f);
	float inv_s = 1.f / s;

	return Quat(
		axis.x * inv_s,
		axis.y * inv_s,
		axis.z * inv_s,
		s * 0.5f
	);
}

Vec4 Quat::AsVec4() const
{
	return Vec4(x, y, z, w);
}

Vec3 Quat::ToEuler() const
{
	float t0 = (2.f * ((w * x) + (y * z)));
	float t1 = 1.f - (2.f * ((x * x) + (y * y)));
	float t2 = maths::Clamp(2.f * ((w * y) - (z * x)), -1.f, 1.f);
	float t3 = (2.f * ((w * z) + (x * y)));
	float t4 = 1.f - (2.f * ((y * y) + (z * z)));

	float p = std::asin(t2);
	float yw = std::atan2(t3, t4);
	float r = std::atan2(t0, t1);

	return Vec3(p, yw, r);
}

Quat Quat::Inverse() const
{
	Quat inverse;

	float length_sqr = LengthSqr();

	if (length_sqr > maths::EPSILONf) {
		float length = std::sqrt(length_sqr);

		inverse.x = -x / length;
		inverse.y = -y / length;
		inverse.z = -z / length;
		inverse.w = w / length;
	} else {
		inverse.x = -x;
		inverse.y = -y;
		inverse.z = -z;
		inverse.w = w;
	}

	return inverse;
}

Quat Quat::Slerp(const Quat &to, float t) const
{
	Vec4 a = AsVec4();
	Vec4 b = to.AsVec4();

	float cos_t = a.Dot(b);

	if (cos_t < 0.f) {
		b = b * -1.f;
		cos_t = -cos_t;
	}

	if (cos_t > 0.999f) {
		Vec4 result = a + (b - a) * t;
		float len = result.Length();

		if (len > maths::EPSILONf) {
			result = result * (1.f / len);
			return Quat(result.x, result.y, result.z, result.w);
		}

		return *this;
	}

	float theta = std::acos(cos_t);
	float sin_t = std::sin(theta);

	float scale_a = std::sin((1.f - t) * theta) / sin_t;
	float scale_b = std::sin((t)*theta) / sin_t;

	Vec4 result = a * scale_a + b * scale_b;

	return Quat(result.x, result.y, result.z, result.w);
}
