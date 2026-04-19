#ifndef APP_H
#define APP_H

#define APP_TARGET_FPS 144
#define APP_HOT_RELOAD_INTERVAL 1.f

typedef struct App App;
struct App
{
	Arena *permanent_arena;
	
	Arena frame_arena;
	Arena scene_arena;

	CH_Timer elapsed_timer;
	CH_Timer delta_timer;
	CH_Timer hot_reload_timer;

	f32 delta_accumulator;

	GM_Stack game_mode_stack;

	AST_Assets assets;

	ENT_World world;
	ENT_EventQueue events;

	GFX_Device graphics_device;
	GFX_Swapchain swapchain;
	GFX_ShaderCompiler shader_compiler;
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

internal void AppInitAudio           (App *app);
internal void AppDestroyAudio        (App *app);
internal void AppHotLoadAudio        (App *app);
internal void AppHotUnloadAudio      (App *app);

internal void AppInitGraphics        (App *app);
internal void AppDestroyGraphics     (App *app);
internal void AppHotLoadGraphics     (App *app);
internal void AppHotUnloadGraphics   (App *app);

internal void AppInitEntity          (App *app);
internal void AppDestroyEntity       (App *app);
internal void AppHotLoadEntity       (App *app);
internal void AppHotUnloadEntity     (App *app);

internal void AppRender(App *app, f32 dt, f32 elapsed, GFX_CmdBuffer *cmd);

__declspec(dllexport) App  *AppInit      (Arena *arena, const OS_API *api);
__declspec(dllexport) void  AppDestroy   (App *app);
__declspec(dllexport) b32   AppTick      (App *app, const I_State *input);
__declspec(dllexport) void  AppHotLoad   (App *app, const OS_API *api);
__declspec(dllexport) void  AppHotUnload (App *app);

#endif // APP_H
