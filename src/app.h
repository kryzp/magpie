#ifndef APP_H
#define APP_H

#define APP_TARGET_FPS 144
#define APP_HOT_RELOAD_INTERVAL 1.f

typedef enum AppMemoryPartition
{
#define Partition(name, ratio) AppMemoryPartition_##name,
#include "partitions.inc"
#undef Partition
	AppMemoryPartition_COUNT
}
AppMemoryPartition;

typedef struct App App;
struct App
{
	Arena partitions[AppMemoryPartition_COUNT];

	Arena graph_arena;
	Arena scene_arena;
	Arena pass_frame_arena;

	LOG_Logger logger;
	LOG_Channel log_channel;

	LOG_Channel graphics_log_channel;
	GFX_Device graphics_device;
	GFX_Swapchain swapchain;
	GFX_RingBuffer frame_upload_ring_buffer;
	GFX_BufferKey frame_data_buffer;
	GFX_BufferKey cubemap_capture_transform_buffer;
	GFX_SamplerKey linear_sampler;
	
	// todo: merge shader compiler into graphics device?
	LOG_Channel shader_compiler_log_channel;
	GFX_ShaderCompiler shader_compiler;
	
	LOG_Channel audio_log_channel;
	AUD_System audio_system;
	AUD_BackendAPI *audio_backend;
	AUD_BufferHandle test_sound;

	LOG_Channel asset_log_channel;
	AST_Assets assets;

	// todo: move rendering stuff into a
	//       seperate manager sub-system.
	LOG_Channel render_log_channel;
	R_Graph graph;
	R_GraphTexHandle swapchain_src;
	R_Scene scene;
	R_SceneLightHandle light_handle;
	R_Camera camera;
	R_Mesh skybox_mesh;

	GFX_TextureKey environment_cubemap;
	GFX_TextureKey irradiance_cubemap;
	GFX_TextureKey prefilter_cubemap;

	ENT_World world;
	ENT_EventQueue events;

	GM_Stack game_mode_stack;

	CameraDriver camera_driver;
	b32 camera_driver_active;
	
	CH_Timer elapsed_timer;
	CH_Timer delta_timer;
	CH_Timer hot_reload_timer;

	f32 delta_accumulator;
};

internal void AppInitLog             (App *app);
internal void AppDestroyLog          (App *app);
internal void AppHotLoadLog          (App *app);
internal void AppHotUnloadLog        (App *app);

internal void AppInitGraphics        (App *app);
internal void AppDestroyGraphics     (App *app);
internal void AppHotLoadGraphics     (App *app);
internal void AppHotUnloadGraphics   (App *app);

internal void AppInitAudio           (App *app);
internal void AppDestroyAudio        (App *app);
internal void AppHotLoadAudio        (App *app);
internal void AppHotUnloadAudio      (App *app);

internal void AppInitAssets          (App *app);
internal void AppDestroyAssets       (App *app);
internal void AppHotLoadAssets       (App *app);
internal void AppHotUnloadAssets     (App *app);

internal void AppInitRender          (App *app);
internal void AppDestroyRender       (App *app);
internal void AppHotLoadRender       (App *app);
internal void AppHotUnloadRender     (App *app);

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

global const OS_API *osapi;

#endif // APP_H
