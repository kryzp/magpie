#ifndef APP_H
#define APP_H

#define APP_TARGET_FPS 144
#define APP_HOT_RELOAD_INTERVAL 1.f

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

	AUD_System audio_system;
	AUD_BackendAPI *audio_backend;
	AUD_BufferHandle test_sound;
};

global const OS_API *osapi;

internal void AppInitAudio      (App *app);
internal void AppDestroyAudio   (App *app);
internal void AppTickAudio      (App *app, f32 dt, f32 elapsed);
internal void AppHotLoadAudio   (App *app);
internal void AppHotUnloadAudio (App *app);

internal void AppInitGraphics      (App *app);
internal void AppDestroyGraphics   (App *app);
internal void AppTickGraphics      (App *app, f32 dt, f32 elapsed);
internal void AppHotLoadGraphics   (App *app);
internal void AppHotUnloadGraphics (App *app);

internal void AppUpdate      (App *app, f32 dt, f32 elapsed, const I_InputSt *input);
internal void AppFixedUpdate (App *app, f32 dt, f32 elapsed, const I_InputSt *input);
internal void AppRender      (App *app, f32 dt, f32 elapsed, GFX_CmdBuffer *cmd);

__declspec(dllexport) void *AppInit      (Arena *arena, const OS_API *api);
__declspec(dllexport) void  AppDestroy   (void *ctx);
__declspec(dllexport) b32   AppTick      (void *ctx, const I_InputSt *input);
__declspec(dllexport) void  AppHotLoad   (void *ctx, const OS_API *api);
__declspec(dllexport) void  AppHotUnload (void *ctx);

#endif // APP_H
