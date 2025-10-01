#include "camera.h"

struct gfx_camera gfx_camera_init_perspective(v3 position, v3 forward, float fov, float aspect, float near, float far)
{
	struct gfx_camera camera = {0};
	camera.type = GFX_CAMERA_TYPE_perspective;
	camera.position = position;
	camera.forward = forward;
	camera.up = v3(0.f, 0.f, 1.f);
	camera.fov = fov;
	camera.aspect = aspect;
	camera.near_plane = near;
	camera.far_plane = far;

	gfx_camera_recompute(&camera);

	return camera;
}

struct gfx_camera gfx_camera_init_orthographic(v3 position, v3 forward, v4 rect, float near, float far)
{
	struct gfx_camera camera = {0};
	camera.type = GFX_CAMERA_TYPE_orthographic;
	camera.position = position;
	camera.forward = forward;
	camera.up = v3(0.f, 0.f, 1.f);
	camera.ortho = rect;
	camera.near_plane = near;
	camera.far_plane = far;

	gfx_camera_recompute(&camera);

	return camera;
}
				  
void gfx_camera_recompute(struct gfx_camera *camera)
{
	camera->view = m4_lookat(camera->position,
				 v3_add(camera->position, camera->forward),
				 camera->up);

	switch (camera->type) {
	case GFX_CAMERA_TYPE_perspective:
		camera->projection = m4_perspective(camera->fov,
						    camera->aspect,
						    camera->near_plane,
						    camera->far_plane);
		break;
		
	case GFX_CAMERA_TYPE_orthographic:
		camera->projection = m4_orthographic(camera->ortho.x, camera->ortho.x + camera->ortho.width,
						     camera->ortho.y, camera->ortho.y + camera->ortho.height,
						     camera->near_plane, camera->far_plane);
		break;
	}
}
