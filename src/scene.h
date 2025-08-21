
typedef enum CameraType
{
	CameraType_Perspective,
	CameraType_Orthographic
}
CameraType;

typedef struct Camera
{
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
}
Camera;

typedef enum LightType
{
	LightType_Point
}
LightType;

typedef struct Light
{
	LightType type;
	v3 position;
	v3 direction;
	v3 colour;
	f32 intensity;
	f32 range;
}
Light;
