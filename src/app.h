#ifndef APP_H
#define APP_H

typedef struct CameraDriver CameraDriver;
struct CameraDriver
{
	f32 yaw;
	f32 target_yaw;
	f32 pitch;
	f32 target_pitch;
};

internal void CameraDriverDrive(CameraDriver *driver, R_Camera *camera);

typedef struct App App;
struct App
{
	Arena *permanent_arena;
	Arena frame_arena;

	CH_Timer global_timer;
	CH_Timer delta_timer;
	CH_Timer hot_reload_timer;

	f32 delta_accumulator;

	AST_Assets assets;

	AUD_System audio_system;
	AUD_BackendAPI *audio_backend;

	AUD_BufferHandle test_sound;

	GFX_Device graphics_device;
	GFX_Swapchain swapchain;

	GFX_RingBuffer frame_upload_ring_buffer;
	GFX_BufferKey frame_data_buffer;
	GFX_BufferKey cubemap_capture_transform_buffer;
	
	R_Graph graph;
	R_GraphTexHandle swapchain_src;
	R_Scene scene;
	R_SceneLightHandle light_handle;
	R_Camera camera;

	CameraDriver camera_driver;
	b32 camera_driver_active;
};

global const OS_API *osapi;

__declspec(dllexport) void *AppInit      (Arena *arena, const OS_API *api);
__declspec(dllexport) void  AppDestroy   (void *ctx);
__declspec(dllexport) b32   AppTick      (void *ctx, const I_InputSt *input);
__declspec(dllexport) void  AppHotLoad   (void *ctx, const OS_API *api);
__declspec(dllexport) void  AppHotUnload (void *ctx);

#endif // APP_H
