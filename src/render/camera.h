#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

#include "core/vec4.h"
#include "core/vec3.h"
#include "core/rect.h"
#include "core/mat4.h"

namespace rnd {

struct FrustumVolume {
	Vec4 planes[6];
};

enum CameraType {
	CameraType_Perspective,
	CameraType_Orthographic,
	CameraType_MaxEnum
};

struct Camera {
	CameraType type = CameraType_Perspective;

	Vec3 position;
	Vec3 forward;
	Vec3 up;

	float fov = 0.f;
	float aspect = 0.f;

	Rect ortho;

	float near_plane = 0.f;
	float far_plane = 0.f;

	Mat4 view;
	Mat4 proj;
	Mat4 view_proj;
	Mat4 view_proj_no_translation;
	Mat4 inv_view;
	Mat4 inv_proj;
	Mat4 inv_view_proj;

	static Camera Perspective(const Vec3 &position, const Vec3 &forward, float fov, float aspect, float near_plane, float far_plane);
	static Camera Orthographic(const Vec3 &position, const Vec3 &forward, const Rect &rect, float near_plane, float far_plane);

	void Recompute();

	FrustumVolume Frustum() const;

	Vec3 NDCToWsRayDirection(const Vec2 &ndc) const;
};

}

#endif // RENDER_CAMERA_H
