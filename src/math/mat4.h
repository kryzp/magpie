#pragma once

#include "vec3.h"
#include "vec4.h"
#include "quat.h"

struct Mat4 {
	union {
		float v[4][4];
		Vec4 c[4];

		struct {
			float m00, m10, m20, m30;
			float m01, m11, m21, m31;
			float m02, m12, m22, m32;
			float m03, m13, m23, m33;
		};
	};

	Mat4();
	Mat4(float dia);

	Mat4(
		float m00, float m10, float m20, float m30,
		float m01, float m11, float m21, float m31,
		float m02, float m12, float m22, float m32,
		float m03, float m13, float m23, float m33
	);

	static Mat4 identity();
	static Mat4 lookat(const Vec3 &eye, const Vec3 &centre, const Vec3 &up);
	static Mat4 perspective(float fov, float aspect, float near, float far);
	static Mat4 orthographic(float left, float right, float bottom, float top, float near, float far);

	static Mat4 rotate_axis(float angle, const Vec3 &axis);
	static Mat4 rotate_quat(const Quat &q);

	static Mat4 translate(const Vec3 &translation);
	static Mat4 scale(const Vec3 &scale);

	static Mat4 transform(
		const Vec3 &position,
		const Quat &rotation,
		const Vec3 &scale,
		const Vec3 &origin
	);

	Mat4 remove_translation() const;
	Mat4 remove_rotation() const;

	Mat4 inverse() const;
	Mat4 transpose() const;

	Mat4 operator + (const Mat4 &other) const;
	Mat4 operator - (const Mat4 &other) const;

	Mat4 operator * (const Mat4 &other) const;
	Vec3 operator * (const Vec3 &vector) const;
	Mat4 operator * (float scalar) const;
};
