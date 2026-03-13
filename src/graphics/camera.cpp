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
	camera.type = TYPE_PERSPECTIVE;
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
	camera.type = TYPE_ORTHOGRAPHIC;
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
	: type()
	, position()
	, forward()
	, up()
	, fov()
	, aspect()
	, ortho()
	, near_plane()
	, far_plane()
	, view()
	, proj()
{
}

Camera::~Camera()
{
}

void Camera::recompute()
{
	view = Mat4::lookat(position, position + forward, up);

	if (type == TYPE_PERSPECTIVE) {
		proj = Mat4::perspective(
			fov, aspect,
			near_plane, far_plane
		);
	} else if (type == TYPE_ORTHOGRAPHIC) {
		proj = Mat4::orthographic(
			ortho.x, ortho.x + ortho.w,
			ortho.y, ortho.y + ortho.h,
			near_plane, far_plane
		);
	}
}

FrustumVolume Camera::frustum_volume() const
{
	Mat4 vpt = (get_projection() * get_view()).transpose();

	FrustumVolume volume = {};

	volume.frustum_planes[0] = (vpt.c[3] + vpt.c[0]).frustum_normalize_plane(); // left
	volume.frustum_planes[1] = (vpt.c[3] - vpt.c[0]).frustum_normalize_plane(); // right
	volume.frustum_planes[2] = (vpt.c[3] + vpt.c[1]).frustum_normalize_plane(); // bottom
	volume.frustum_planes[3] = (vpt.c[3] - vpt.c[1]).frustum_normalize_plane(); // top
	volume.frustum_planes[4] = (           vpt.c[2]).frustum_normalize_plane(); // near
	volume.frustum_planes[5] = (vpt.c[3] - vpt.c[2]).frustum_normalize_plane(); // far

	return volume;
}
