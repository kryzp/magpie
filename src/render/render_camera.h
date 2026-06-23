#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

typedef struct R_FrustumVolume R_FrustumVolume;
struct R_FrustumVolume
{
	v4 planes[6];
};

typedef enum R_CameraType
{
	R_CameraType_Perspective,
	R_CameraType_Orthographic,
	R_CameraType_COUNT
}
R_CameraType;

typedef struct R_Camera R_Camera;
struct R_Camera
{
	R_CameraType type;

	v3 position;
	v3 forward;
	v3 up;

	f32 fov;
	f32 aspect;

	v4 ortho;

	f32 near_plane;
	f32 far_plane;

	m4 view;
	m4 proj;
};

static R_Camera R_CameraPerspective(v3 position, v3 forward, f32 fov, f32 aspect, f32 near, f32 far);
static R_Camera R_CameraOrthographic(v3 position, v3 forward, v4 rect, f32 near, f32 far);

static void R_CameraRecompute(R_Camera *camera);

static R_FrustumVolume R_CameraFrustum(const R_Camera *camera);

#endif // RENDER_CAMERA_H
