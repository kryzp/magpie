#ifndef CAMERA_H
#define CAMERA_H

#include "core/core_math.h"

enum gfx_camera_type {
	GFX_CAMERA_TYPE_perspective,
	GFX_CAMERA_TYPE_orthographic
};

struct gfx_camera {
	enum gfx_camera_type type;

	v3 position;
	v3 forward;
	v3 up;

	float fov;
	float aspect;

	v4 ortho;
	
	float near_plane;
	float far_plane;

	m4 view;
	m4 projection;
};

struct gfx_camera gfx_camera_init_perspective(v3 position, v3 forward, float fov, float aspect, float near, float far);
struct gfx_camera gfx_camera_init_orthographic(v3 position, v3 forward, v4 rect, float near, float far);

void gfx_camera_recompute(struct gfx_camera *camera);

#endif // CAMERA_H
