#pragma once

#include "vec3.h"

struct Quat {
	float x;
	float y;
	float z;
	float w;

	Quat();
	Quat(float w);
	Quat(float x, float y, float z, float w);

	static Quat identity();
	static Quat from_axis(float angle, const Vec3 &axis);
	static Quat from_euler(float pitch, float yaw, float roll);
	static Vec3 to_euler(const Quat &quat);

	Quat inverse() const;
};
