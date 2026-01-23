#include "vec4.h"

#include "calc.h"

Vec4::Vec4()
	: x(0.f), y(0.f), z(0.f), w(0.f)
{
}

Vec4::Vec4(float s)
	: x(s), y(s), z(s), w(s)
{
}

Vec4::Vec4(float x, float y, float z, float w)
	: x(x), y(y), z(z), w(w)
{
}

Vec4 Vec4::frustum_normalize_plane() const
{
	float length = CalcF::sqrt(x*x + y*y + z*z);
	return *this / length;
}

Vec3 Vec4::get_xyz() const
{
	return Vec3(x, y, z);
}

bool Vec4::operator == (const Vec4 &other) const { return this->x == other.x && this->y == other.y && this->z == other.z && this->w == other.w; }
bool Vec4::operator != (const Vec4 &other) const { return !(*this == other); }

Vec4 Vec4::operator + (const Vec4 &other) const { return Vec4( this->x + other.x,  this->y + other.y, this->z + other.z, this->w + other.w); }
Vec4 Vec4::operator - (const Vec4 &other) const { return Vec4( this->x - other.x,  this->y - other.y, this->z - other.z, this->w - other.w); }
Vec4 Vec4::operator * (const Vec4 &other) const { return Vec4( this->x * other.x,  this->y * other.y, this->z * other.z, this->w * other.w); }
Vec4 Vec4::operator / (const Vec4 &other) const { return Vec4( this->x / other.x,  this->y / other.y, this->z / other.z, this->w / other.w); }

Vec4 Vec4::operator - () const { return Vec4(-this->x, -this->y, -this->z, -this->w); }

Vec4 &Vec4::operator += (const Vec4 &other) { this->x += other.x; this->y += other.y; this->z += other.z; this->w += other.w; return *this; }
Vec4 &Vec4::operator -= (const Vec4 &other) { this->x -= other.x; this->y -= other.y; this->z -= other.z; this->w -= other.w; return *this; }
Vec4 &Vec4::operator *= (const Vec4 &other) { this->x *= other.x; this->y *= other.y; this->z *= other.z; this->w *= other.w; return *this; }
Vec4 &Vec4::operator /= (const Vec4 &other) { this->x /= other.x; this->y /= other.y; this->z /= other.z; this->w /= other.w; return *this; }
