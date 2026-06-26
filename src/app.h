#ifndef APP_H
#define APP_H

#define APP_TARGET_FPS                144
#define APP_HOT_RELOAD_INTERVAL       1.f

typedef struct App App;
struct App
{
	Arena bootstrap_arena;
	Arena log_arena;
	Arena graphics_arena;
	Arena scripting_arena;
	Arena audio_arena;
	Arena asset_arena;
	Arena animation_arena;
	Arena render_arena;
	Arena physics_arena;
	Arena entity_arena;
	Arena editor_arena;
	Arena frame_arena;
	
	LOG_Channel log_channel;
	LOG_Channel scripting_log_channel;
	LOG_Channel graphics_log_channel;
	LOG_Channel audio_log_channel;
	LOG_Channel asset_log_channel;
	LOG_Channel animation_log_channel;
	LOG_Channel render_log_channel;
	LOG_Channel physics_log_channel;
	LOG_Channel entity_log_channel;

	S_System *scripting_system;
	
	G_Device graphics_device;
	G_Swapchain swapchain;
	G_ShaderCompiler shader_compiler;
	
	AU_System audio_system;
	AU_Backend *audio_backend;
	A_Handle test_sound_handle;
	AU_BufferHandle test_sound;

	A_Assets assets;

	AN_System animation_system;

	G_RingBuffer frame_upload_ring_buffer;
	R_Graph graph;
	R_Scene scene;
	R_System render_system;
	R_FrameParams prev_frame;
	u64 frame_count;
	R_SceneHandle light_handle;

	P_Engine physics_engine;

	E_World world;
	E_EventQueue events;

	Game game;

	CH_Timer elapsed_timer;
	CH_Timer delta_timer;
	CH_Timer hot_reload_timer;
	f32 delta_accumulator;
};

__declspec(dllexport) App  *AppInit      (const OS_API *api);
__declspec(dllexport) void  AppDestroy   (App *app);
__declspec(dllexport) b32   AppTick      (App *app, const OS_InputState *input);
__declspec(dllexport) void  AppHotLoad   (App *app, const OS_API *api);
__declspec(dllexport) void  AppHotUnload (App *app);

#endif // APP_H
