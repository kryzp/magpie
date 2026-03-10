#pragma once

#include "vec2.h"

struct Vec3
{
	union {
		struct {
			float x;
			float y;
			float z;
		};
		float v[3];
	};

	Vec3();
	Vec3(float s);
	Vec3(float x, float y, float z);
	explicit Vec3(const Vec2 &xy);

	static const Vec3 &unit();
	static const Vec3 &zero();
	static const Vec3 &one();
	static const Vec3 &right();
	static const Vec3 &left();
	static const Vec3 &up();
	static const Vec3 &down();
	static const Vec3 &forward();
	static const Vec3 &backward();

	static Vec3 spherical_to_cartesian(float radius, float azimuth, float elevation);
	
	static Vec3 min(const Vec3 &a, const Vec3 &b);
	static Vec3 max(const Vec3 &a, const Vec3 &b);

	static float dot(const Vec3 &a, const Vec3 &b);
	static Vec3 cross(const Vec3 &a, const Vec3 &b);
	
	static Vec3 lerp(const Vec3 &from, const Vec3 &to, float amount);
	static Vec3 approach(const Vec3 &from, const Vec3 &to, float amount);
	
	static Vec3 reflect(const Vec3 &v, const Vec3 &n);
	static Vec3 refract(const Vec3 &uv, const Vec3 &n, double eta21);
	
	float length() const;
	float length_squared() const;

	float min_value() const;
	float max_value() const;

	Vec3 abs() const;
	Vec3 normalized() const;
	Vec2 xy() const;

	bool operator == (const Vec3 &other) const;
	bool operator != (const Vec3 &other) const;

	Vec3 operator + (const Vec3 &other) const;
	Vec3 operator - (const Vec3 &other) const;
	Vec3 operator * (const Vec3 &other) const;
	Vec3 operator / (const Vec3 &other) const;

	Vec3 operator - () const;

	Vec3 &operator += (const Vec3 &other);
	Vec3 &operator -= (const Vec3 &other);
	Vec3 &operator *= (const Vec3 &other);
	Vec3 &operator /= (const Vec3 &other);
};
