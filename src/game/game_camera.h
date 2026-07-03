#ifndef GAME_CAMERA_H
#define GAME_CAMERA_H

typedef enum CameraDriverMode
{
	CameraDriverMode_Unrestricted,
	CameraDriverMode_COUNT
}
CameraDriverMode;

typedef struct CameraDriverConfig CameraDriverConfig;
struct CameraDriverConfig
{
	CameraDriverMode mode;
	
	v3 offset;

	f32 fov;

	f32 yaw_min, yaw_max;
	f32 pitch_min, pitch_max;

	f32 shake_factor;
};

typedef struct CameraDriver CameraDriver;
struct CameraDriver
{
	CameraDriverConfig config;
	
	v3 target_position;

	//f32 yaw, target_yaw;
	//f32 pitch, target_pitch;
};

static CameraDriver CameraDriverInit(const CameraDriverConfig *config);

static void CameraDriverShake(CameraDriver *driver, f32 amount);
static void CameraDriverDrive(CameraDriver *driver, R_Camera *camera, const OS_InputState *input, f32 dt);

#endif // GAME_CAMERA_H
