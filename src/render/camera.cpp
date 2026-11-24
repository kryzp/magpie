#include "render/camera.h"
#include "core/vec2.h"

rnd::Camera rnd::Camera::Perspective(const Vec3 &position, const Vec3 &forward, float fov, float aspect, float near_plane, float far_plane)
{
	Camera camera;
	camera.type = CameraType_Perspective;
	camera.position = position;
	camera.forward = forward;
	camera.up = Vec3(0.f, 0.f, 1.f);
	camera.fov = fov;
	camera.aspect = aspect;
	camera.near_plane = near_plane;
	camera.far_plane = far_plane;

	camera.Recompute();

	return camera;
}

rnd::Camera rnd::Camera::Orthographic(const Vec3 &position, const Vec3 &forward, const Rect &rect, float near_plane, float far_plane)
{
	Camera camera;
	camera.type = CameraType_Orthographic;
	camera.position = position;
	camera.forward = forward;
	camera.up = Vec3(0.f, 0.f, 1.f);
	camera.ortho = rect;
	camera.near_plane = near_plane;
	camera.far_plane = far_plane;

	camera.Recompute();

	return camera;
}

void rnd::Camera::Recompute()
{
	view = Mat4::LookAt(position, position + forward, up);

	switch (type) {
		case CameraType_Perspective:
			proj = Mat4::Perspective(fov, aspect, near_plane, far_plane);
			break;

		case CameraType_Orthographic:
			proj = Mat4::Orthographic(
				ortho.x, ortho.x + ortho.width,
				ortho.y, ortho.y + ortho.height,
				near_plane, far_plane
			);
			break;

		default:
			break;
	}

	view_proj = proj * view;
	view_proj_no_translation = proj * view.WithoutTranslation();

	inv_view = view.Inverse();
	inv_proj = proj.Inverse();
	inv_view_proj = view_proj.Inverse();
}

rnd::FrustumVolume rnd::Camera::Frustum() const
{
	Mat4 vpt = view_proj.Transposed();

	FrustumVolume volume;

	volume.planes[0] = (vpt.c[3] + vpt.c[0]).FrustumNormalizePlane();
	volume.planes[1] = (vpt.c[3] - vpt.c[0]).FrustumNormalizePlane();
	volume.planes[2] = (vpt.c[3] + vpt.c[1]).FrustumNormalizePlane();
	volume.planes[3] = (vpt.c[3] - vpt.c[1]).FrustumNormalizePlane();
	volume.planes[4] = (vpt.c[2]).FrustumNormalizePlane();
	volume.planes[5] = (vpt.c[3] - vpt.c[2]).FrustumNormalizePlane();

	return volume;
}

Vec3 rnd::Camera::NDCToWsRayDirection(const Vec2 &ndc) const
{
	Mat4 inverse_proj = proj.Inverse();
	Mat4 inverse_view = view.Inverse();

	Vec4 clip = Vec4(ndc.x, ndc.y, 0.f, 1.f);

	Vec4 view_pos = inverse_proj * clip;

	view_pos.y = 1.f;
	view_pos.w = 0.f;

	Vec4 world_dir4 = inverse_view * view_pos;

	Vec3 world_dir = Vec3(world_dir4.x, world_dir4.y, world_dir4.z);
	world_dir = world_dir.Normalized();

	return world_dir;
}
