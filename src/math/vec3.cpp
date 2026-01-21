#include "vec3.h"
#include "calc.h"

// Global operators.
Vec3 operator * (const Vec3 &lhs, float rhs) { return Vec3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs); }
Vec3 operator * (float lhs, const Vec3 &rhs) { return Vec3(rhs.x * lhs, rhs.y * lhs, rhs.z * lhs); }

Vec3::Vec3()
	: x(0.0), y(0.0), z(0.0)
{
}

Vec3::Vec3(float s)
	: x(s), y(s), z(s)
{
}

Vec3::Vec3(float x, float y, float z)
	: x(x), y(y), z(z)
{
}

Vec3::Vec3(const Vec2 &xy)
	: x(xy.x), y(xy.y), z(0.0)
{
}

Vec3 Vec3::spherical_to_cartesian(float radius, float azimuth, float elevation)
{
	return Vec3(
		radius * CalcF::cos(elevation) * CalcF::cos(azimuth),
		radius * CalcF::cos(elevation) * CalcF::sin(azimuth),
		radius * CalcF::sin(elevation)
	);
}

float Vec3::dot(const Vec3 &a, const Vec3 &b)
{
	return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

Vec3 Vec3::cross(const Vec3 &a, const Vec3 &b)
{
	return Vec3(
		(a.y * b.z) - (a.z * b.y),
		(a.z * b.x) - (a.x * b.z),
		(a.x * b.y) - (a.y * b.x)
	);
}

Vec3 Vec3::lerp(const Vec3 &from, const Vec3 &to, float amount)
{
	return Vec3(
		CalcF::lerp(from.x, to.x, amount),
		CalcF::lerp(from.y, to.y, amount),
		CalcF::lerp(from.z, to.z, amount)
	);
}

Vec3 Vec3::approach(const Vec3 &from, const Vec3 &to, float amount)
{
	return Vec3(
		CalcF::approach(from.x, to.x, amount),
		CalcF::approach(from.y, to.y, amount),
		CalcF::approach(from.z, to.z, amount)
	);
}

Vec3 Vec3::reflect(const Vec3 &v, const Vec3 &n)
{
	return v - (2.f * dot(v, n) * n);
}

Vec3 Vec3::refract(const Vec3 &uv, const Vec3 &n, double eta21)
{
	double cost = CalcF::min(dot(-uv, n), 1.f);
	Vec3 out_perp = eta21 * (uv + (cost * n));
	Vec3 out_para = -CalcF::sqrt(CalcF::abs(1.f - out_perp.length_squared())) * n;
	return out_perp + out_para;
}

float Vec3::length() const
{
	return CalcF::sqrt(length_squared());
}

float Vec3::length_squared() const
{
	return (x * x) + (y * y) + (z * z);
}

float Vec3::min_value() const
{
	return CalcF::min(x, CalcF::min(y, z));
}

float Vec3::max_value() const
{
	return CalcF::max(x, CalcF::max(y, z));
}

Vec3 Vec3::abs() const
{
	return Vec3(
		CalcF::abs(x),
		CalcF::abs(y),
		CalcF::abs(z)
	);
}

Vec3 Vec3::normalized() const
{
	float len = length();

	if (len <= CalcF::epsilon())
		return zero();

	return Vec3(
		x / len,
		y / len,
		z / len
	);
}

Vec2 Vec3::xy() const
{
	return Vec2(x, y);
}

bool Vec3::operator == (const Vec3 &other) const { return this->x == other.x && this->y == other.y && this->z == other.z; }
bool Vec3::operator != (const Vec3 &other) const { return !(*this == other); }

Vec3 Vec3::operator + (const Vec3 &other) const { return Vec3(this->x + other.x, this->y + other.y, this->z + other.z); }
Vec3 Vec3::operator - (const Vec3 &other) const { return Vec3(this->x - other.x, this->y - other.y, this->z - other.z); }
Vec3 Vec3::operator * (const Vec3 &other) const { return Vec3(this->x * other.x, this->y * other.y, this->z * other.z); }
Vec3 Vec3::operator / (const Vec3 &other) const { return Vec3(this->x / other.x, this->y / other.y, this->z / other.z); }

Vec3 Vec3::operator - () const { return Vec3(-this->x, -this->y, -this->z); }

Vec3 &Vec3::operator += (const Vec3 &other) { this->x += other.x; this->y += other.y; this->z += other.z; return *this; }
Vec3 &Vec3::operator -= (const Vec3 &other) { this->x -= other.x; this->y -= other.y; this->z -= other.z; return *this; }
Vec3 &Vec3::operator *= (const Vec3 &other) { this->x *= other.x; this->y *= other.y; this->z *= other.z; return *this; }
Vec3 &Vec3::operator /= (const Vec3 &other) { this->x /= other.x; this->y /= other.y; this->z /= other.z; return *this; }

const Vec3 &Vec3::unit()		{ static const Vec3 UNIT		= Vec3( 1,  1,  1); return UNIT;        }
const Vec3 &Vec3::zero()		{ static const Vec3 ZERO		= Vec3( 0,  0,  0); return ZERO;        }
const Vec3 &Vec3::one()			{ static const Vec3 ONE			= Vec3( 1,  1,  1); return ONE;         }
const Vec3 &Vec3::right()		{ static const Vec3 RIGHT		= Vec3( 1,  0,  0); return RIGHT;       }
const Vec3 &Vec3::left()		{ static const Vec3 LEFT		= Vec3(-1,  0,  0); return LEFT;        }
const Vec3 &Vec3::up()			{ static const Vec3 UP			= Vec3( 0,  0,  1); return UP;          }
const Vec3 &Vec3::down()		{ static const Vec3 DOWN		= Vec3( 0,  0, -1); return DOWN;        }
const Vec3 &Vec3::forward()		{ static const Vec3 FORWARD		= Vec3( 0,  1,  0); return FORWARD;     }
const Vec3 &Vec3::backward()	{ static const Vec3 BACKWARD	= Vec3( 0, -1,  0); return BACKWARD;    }
