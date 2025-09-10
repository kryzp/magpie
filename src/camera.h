
typedef enum CameraType {
	CameraType_Perspective,
	CameraType_Orthographic
} CameraType;

typedef struct Camera {
	CameraType type;

	v3 position;
	v3 forward;
	v3 up;

	f32 fov;
	f32 aspect;
	f32 near_plane;
	f32 far_plane;

	m4 view;
	m4 projection;
} Camera;

typedef struct CameraDriver {
	b32 active;
	f32 yaw;
	f32 pitch;
	f32 target_yaw;
	f32 target_pitch;
} CameraDriver;
