#pragma once

#include "vec3.h"

struct Vec4
{
	union {
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		float v[4];
	};

	Vec4();
	Vec4(float s);
	Vec4(float x, float y, float z, float w);

	Vec3 get_xyz() const;

	bool operator == (const Vec4 &other) const;
	bool operator != (const Vec4 &other) const;

	Vec4 operator + (const Vec4 &other) const;
	Vec4 operator - (const Vec4 &other) const;
	Vec4 operator * (const Vec4 &other) const;
	Vec4 operator / (const Vec4 &other) const;

	Vec4 operator - () const;

	Vec4 &operator += (const Vec4 &other);
	Vec4 &operator -= (const Vec4 &other);
	Vec4 &operator *= (const Vec4 &other);
	Vec4 &operator /= (const Vec4 &other);
};
