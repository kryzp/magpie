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
	
	// ---

	LOG_Channel scripting_log_channel;
	S_System *scripting_system;
	
	// ---

	LOG_Channel graphics_log_channel;
	G_Device graphics_device;
	G_Swapchain swapchain;
	G_ShaderCompiler shader_compiler;
	
	// ---
	
	LOG_Channel audio_log_channel;
	AU_System audio_system;
	AU_Backend *audio_backend;
	A_Handle test_sound_handle;
	AU_BufferHandle test_sound;

	// ---

	LOG_Channel asset_log_channel;
	A_Registry assets;

	// ---

	LOG_Channel animation_log_channel;
	AN_System animation_system;

	// ---

	LOG_Channel render_log_channel;
	G_RingBuffer frame_upload_ring_buffer;
	R_Graph graph;
	R_Scene scene;
	R_System render_system;
	R_FrameParams prev_frame;
	u64 frame_count;
	R_SceneHandle light_handle;

	// ---

	LOG_Channel physics_log_channel;
	P_Engine physics_engine;

	// ---

	LOG_Channel entity_log_channel;
	E_World world;
	E_EventQueue events;

	// ---

	LOG_Channel editor_log_channel;
	Editor editor;

	// ---

	LOG_Channel log_channel;
	CH_Timer elapsed_timer;
	CH_Timer delta_timer;
	CH_Timer hot_reload_timer;
	f32 delta_accumulator;
};

static void AppInitScripting       (App *app);
static void AppDestroyScripting    (App *app);
static void AppHotLoadScripting    (App *app);
static void AppHotUnloadScripting  (App *app);

static void AppInitGraphics        (App *app);
static void AppDestroyGraphics     (App *app);
static void AppHotLoadGraphics     (App *app);
static void AppHotUnloadGraphics   (App *app);

static void AppInitAudio           (App *app);
static void AppDestroyAudio        (App *app);
static void AppHotLoadAudio        (App *app);
static void AppHotUnloadAudio      (App *app);

static void AppInitAssets          (App *app);
static void AppDestroyAssets       (App *app);
static void AppHotLoadAssets       (App *app);
static void AppHotUnloadAssets     (App *app);

static void AppInitAnimation       (App *app);
static void AppDestroyAnimation    (App *app);
static void AppHotLoadAnimation    (App *app);
static void AppHotUnloadAnimation  (App *app);

static void AppInitRender          (App *app);
static void AppDestroyRender       (App *app);
static void AppHotLoadRender       (App *app);
static void AppHotUnloadRender     (App *app);

static void AppInitPhysics         (App *app);
static void AppDestroyPhysics      (App *app);
static void AppHotLoadPhysics      (App *app);
static void AppHotUnloadPhysics    (App *app);

static void AppInitEntity          (App *app);
static void AppDestroyEntity       (App *app);
static void AppHotLoadEntity       (App *app);
static void AppHotUnloadEntity     (App *app);

static void AppInitEditor          (App *app);
static void AppDestroyEditor       (App *app);
static void AppHotLoadEditor       (App *app);
static void AppHotUnloadEditor     (App *app);

__declspec(dllexport) App  *AppInit      (const OS_API *api);
__declspec(dllexport) void  AppDestroy   (App *app);
__declspec(dllexport) b32   AppTick      (App *app, const OS_InputState *input);
__declspec(dllexport) void  AppHotLoad   (App *app, const OS_API *api);
__declspec(dllexport) void  AppHotUnload (App *app);

#endif // APP_H
