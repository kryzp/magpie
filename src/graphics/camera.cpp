#include "graphics/camera.h"

using namespace gfx;

Camera Camera::perspective(
	const Vec3 &position,
	const Vec3 &forward,
	float fov, float aspect,
	float near, float far
)
{
	Camera camera;
	camera.type = CAMERA_PERSPECTIVE;
	camera.position = position;
	camera.forward = forward;
	camera.up = Vec3::up();
	camera.fov = fov;
	camera.aspect = aspect;
	camera.near_plane = near;
	camera.far_plane = far;

	camera.recompute();

	return camera;
}

Camera Camera::orthographic(
	const Vec3 &position,
	const Vec3 &forward,
	const Rect &rect,
	float near, float far
)
{
	Camera camera;
	camera.type = CAMERA_ORTHOGRAPHIC;
	camera.position = position;
	camera.forward = forward;
	camera.up = Vec3::up();
	camera.ortho = rect;
	camera.near_plane = near;
	camera.far_plane = far;

	camera.recompute();

	return camera;
}

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::recompute()
{
	view = Mat4::lookat(position, position + forward, up);

	if (type == CAMERA_PERSPECTIVE) {
		proj = Mat4::perspective(
			fov, aspect,
			near_plane, far_plane
		);
	} else if (type == CAMERA_ORTHOGRAPHIC) {
		proj = Mat4::orthographic(
			ortho.x, ortho.x + ortho.w,
			ortho.y, ortho.y + ortho.h,
			near_plane, far_plane
		);
	}
}
