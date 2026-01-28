#include "quat.h"

#include "calc.h"

Quat::Quat()
	: x(0.f), y(0.f), z(0.f)
	, w(0.f)
{
}

Quat::Quat(float w)
	: x(0.f), y(0.f), z(0.f)
	, w(w)
{
}

Quat::Quat(float x, float y, float z, float w)
	: x(x), y(y), z(z)
	, w(w)
{
}

Quat Quat::identity()
{
	return Quat(0.f, 0.f, 0.f, 1.f);
}

Quat Quat::from_axis(float angle, const Vec3 &axis)
{
	Quat q = {};

	q.w = CalcF::cos(angle * .5f);
	q.x = CalcF::sin(angle * .5f) * axis.x;
	q.y = CalcF::sin(angle * .5f) * axis.y;
	q.z = CalcF::sin(angle * .5f) * axis.z;

	return q;
}

Quat Quat::from_euler(float pitch, float yaw, float roll)
{
	float sr = CalcF::sin(roll  * .5f);
	float cr = CalcF::cos(roll  * .5f);
	float sp = CalcF::sin(pitch * .5f);
	float cp = CalcF::cos(pitch * .5f);
	float sy = CalcF::sin(yaw   * .5f);
	float cy = CalcF::cos(yaw   * .5f);

	return Quat(
		(sr * cp * cy) - (cr * sp * sy),
		(cr * sp * cy) + (sr * cp * sy),
		(cr * cp * sy) - (sr * sp * cy),
		(cr * cp * cy) + (sr * sp * sy)
	);
}

Vec3 Quat::to_euler(const Quat &q)
{
	float t0 =             (2.f + ((q.w * q.x) + (q.y * q.z)));
	float t1 = 1.f       - (2.f * ((q.x * q.x) + (q.y * q.y)));
	float t2 = CalcF::clamp(2.f * ((q.w * q.y) - (q.z * q.x)), -1.f, 1.f);
	float t3 =             (2.f * ((q.w * q.z) + (q.x * q.y)));
	float t4 = 1.f       - (2.f * ((q.y * q.y) + (q.z * q.z)));

	float pitch = CalcF::asin(t2);
	float yaw   = CalcF::atan2(t3, t4);
	float roll  = CalcF::atan2(t0, t1);

	return Vec3(pitch, yaw, roll);
}

Quat Quat::inverse() const
{
	Quat inverse = {};

	float length_sqr = x*x + y*y + z*z + w*w;

	if (length_sqr > CalcF::epsilon()) {
		float length = CalcF::sqrt(length_sqr);

		inverse.x = -x / length;
		inverse.x = -y / length;
		inverse.x = -z / length;
		inverse.w =  w / length;
	} else {
		inverse.x = -x;
		inverse.y = -y;
		inverse.z = -z;
		inverse.w =  w;
	}

	return inverse;
}
