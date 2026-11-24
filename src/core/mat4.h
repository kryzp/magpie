#ifndef CORE_MAT4_H
#define CORE_MAT4_H

#include "vec4.h"

struct Quat;
struct Vec3;

struct Mat4 {
	union {
		struct {
			float m00, m10, m20, m30;
			float m01, m11, m21, m31;
			float m02, m12, m22, m32;
			float m03, m13, m23, m33;
		};

		Vec4 c[4];
		float e[4][4];
	};

	Mat4();
	explicit Mat4(float diagonal);

	static Mat4 Identity()
	{
		return Mat4(1.f);
	}

	static Mat4 LookAt(const Vec3 &eye, const Vec3 &centre, const Vec3 &up);

	static Mat4 Perspective(float fov, float aspect, float near_plane, float far_plane);
	static Mat4 Orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);

	// Orthographic projection for 2D drawing (canvases, overlays): x right,
	// y up, looking down -z, with depth mapped onto Vulkan's 0..1 range.
	static Mat4 Orthographic2D(float left, float right, float bottom, float top, float near_plane, float far_plane);

	static Mat4 Translate(const Vec3 &translation);
	static Mat4 Scale(const Vec3 &scale);

	static Mat4 RotateAxis(float angle, const Vec3 &axis); // . . Must be normalized.
	static Mat4 RotateQuat(const Quat &q); // . . . . . . . . . . Must be normalized.

	static Mat4 Transform(const Vec3 &position, const Quat &rotation, const Vec3 &scale, const Vec3 &origin);

	[[nodiscard]] Vec3 TransformPoint(const Vec3 &v) const;

	[[nodiscard]] Mat4 Lerp(const Mat4 &to, float t) const;

	[[nodiscard]] Mat4 Inverse() const;
	[[nodiscard]] Mat4 Transposed() const;

	[[nodiscard]] Mat4 WithoutTranslation() const;
	[[nodiscard]] Mat4 WithoutRotation() const;

	[[nodiscard]] Vec3 GetTranslation() const;

	bool operator==(const Mat4 &other) const;

	bool operator!=(const Mat4 &other) const
	{
		return !(*this == other);
	}

	Mat4 operator*(const Mat4 &other) const;
	Vec4 operator*(const Vec4 &v) const;
	Mat4 operator*(float f) const;
};

#endif // CORE_MAT4_H
