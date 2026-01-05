#include "vec2.h"
#include "calc.h"

// global operators
Vec2 operator * (const Vec2 &lhs, float rhs) { return Vec2(lhs.x * rhs, lhs.y * rhs); }
Vec2 operator * (float lhs, const Vec2 &rhs) { return Vec2(rhs.x * lhs, rhs.y * lhs); }

Vec2::Vec2()
	: x(0.0), y(0.f)
{
}

Vec2::Vec2(float s)
	: x(s), y(s)
{
}

Vec2::Vec2(float x, float y)
	: x(x), y(y)
{
}

float Vec2::dot(const Vec2 &a, const Vec2 &b)
{
	return (a.x * b.x) + (a.y * b.y);
}

float Vec2::cross(const Vec2 &a, const Vec2 &b)
{
	return (a.x * b.y) - (a.y * b.x);
}

Vec2 Vec2::from_angle(float radius, float angle)
{
	return Vec2(
		radius * CalcF::cos(angle),
		radius * CalcF::sin(angle)
	);
}

Vec2 Vec2::lerp(const Vec2 &from, const Vec2 &to, float amount)
{
	return Vec2(
		CalcF::lerp(from.x, to.x, amount),
		CalcF::lerp(from.y, to.y, amount)
	);
}

Vec2 Vec2::approach(const Vec2 &from, const Vec2 &to, float amount)
{
	return Vec2(
		CalcF::approach(from.x, to.x, amount),
		CalcF::approach(from.y, to.y, amount)
	);
}

float Vec2::angle() const
{
	return CalcF::atan2(y, x);
}

float Vec2::length() const
{
	return CalcF::sqrt(length_squared());
}

float Vec2::length_squared() const
{
	return (x * x) + (y * y);
}

float Vec2::min_value() const
{
	return CalcF::min(x, y);
}

float Vec2::max_value() const
{
	return CalcF::max(x, y);
}

Vec2 Vec2::abs() const
{
	return Vec2(
		CalcF::abs(this->x),
		CalcF::abs(this->y)
	);
}

Vec2 Vec2::normalized() const
{
	float len = length();

	return Vec2(
		this->x / len,
		this->y / len
	);
}

Vec2 Vec2::perpendicular() const
{
	return Vec2(-this->y, this->x);
}

bool Vec2::operator == (const Vec2 &other) const { return this->x == other.x && this->y == other.y; }
bool Vec2::operator != (const Vec2 &other) const { return !(*this == other); }

Vec2 Vec2::operator + (const Vec2 &other) const { return Vec2( this->x + other.x,  this->y + other.y); }
Vec2 Vec2::operator - (const Vec2 &other) const { return Vec2( this->x - other.x,  this->y - other.y); }
Vec2 Vec2::operator - ()                  const { return Vec2(-this->x,           -this->y          ); }
Vec2 Vec2::operator * (const Vec2 &other) const { return Vec2( this->x * other.x,  this->y * other.y); }
Vec2 Vec2::operator / (const Vec2 &other) const { return Vec2( this->x / other.x,  this->y / other.y); }

Vec2 &Vec2::operator += (const Vec2 &other) { this->x += other.x; this->y += other.y; return *this; }
Vec2 &Vec2::operator -= (const Vec2 &other) { this->x -= other.x; this->y -= other.y; return *this; }
Vec2 &Vec2::operator *= (const Vec2 &other) { this->x *= other.x; this->y *= other.y; return *this; }
Vec2 &Vec2::operator /= (const Vec2 &other) { this->x /= other.x; this->y /= other.y; return *this; }

const Vec2 &Vec2::unit()	{ static const Vec2 UNIT	= Vec2( 1,  1); return UNIT;	}
const Vec2 &Vec2::zero()	{ static const Vec2 ZERO	= Vec2( 0,  0); return ZERO;	}
const Vec2 &Vec2::one()		{ static const Vec2 ONE		= Vec2( 1,  1); return ONE;	}
const Vec2 &Vec2::left()	{ static const Vec2 LEFT	= Vec2(-1,  0); return LEFT;	}
const Vec2 &Vec2::right()	{ static const Vec2 RIGHT	= Vec2( 1,  0); return RIGHT;	}
const Vec2 &Vec2::up()		{ static const Vec2 UP		= Vec2( 0, -1); return UP;	}
const Vec2 &Vec2::down()	{ static const Vec2 DOWN	= Vec2( 0,  1); return DOWN;	}
