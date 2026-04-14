#ifndef CAMERA_DRIVER_H
#define CAMERA_DRIVER_H

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
	
	f32 yaw, target_yaw;
	f32 pitch, target_pitch;
};

internal CameraDriver CameraDriverInit(const CameraDriverConfig *config);

internal void CameraDriverShake(CameraDriver *driver, f32 amount);

internal void CameraDriverDrive(CameraDriver *driver, R_Camera *camera);

#endif // CAMERA_DRIVER_H
