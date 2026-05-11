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
	Arena audio_arena;
	Arena asset_arena;
	Arena render_arena;
	Arena physics_arena;
	Arena entity_arena;
	Arena editor_arena;

	Arena frame_arena;
	
	// ---

	LOG_Channel graphics_log_channel;
	
	GFX_Device graphics_device;
	GFX_Swapchain swapchain;
	GFX_RingBuffer frame_upload_ring_buffer;
	GFX_BufferKey frame_data_buffer;
	GFX_BufferKey cubemap_capture_transform_buffer;
	GFX_SamplerKey linear_sampler;
	GFX_SamplerKey nearest_sampler;
	
	// ---
	
	// todo: merge shader compiler into graphics device?
	LOG_Channel shader_compiler_log_channel;
	
	GFX_ShaderCompiler shader_compiler;

	// ---
	
	LOG_Channel audio_log_channel;
	
	AUD_System audio_system;
	AUD_BackendAPI *audio_backend;
	AUD_BufferHandle test_sound;

	// ---

	LOG_Channel asset_log_channel;
	
	AST_Assets assets;

	// ---

	// todo: move rendering stuff into a
	//       seperate manager sub-system.
	LOG_Channel render_log_channel;
	
	R_Graph graph;
	R_Scene scene;

	R_Culling culling;
	R_ShadowRenderer shadow_renderer;
	R_ForwardRenderer forward_renderer;
	R_DebugRenderer debug_renderer;
	R_IrradianceVolume irradiance_volume;
	R_SSAO ssao;

	AST_Handle object_model_handle;
	R_SceneObjectHandle object_handle;
	m4 object_palette[256];
	
	R_SceneLightHandle light_handle;
	
	R_Mesh skybox_mesh;
	
	GFX_TextureKey brdf_lut;
	GFX_TextureKey environment_cubemap;
	GFX_TextureKey irradiance_cubemap;
	GFX_TextureKey prefilter_cubemap;

	// ---

	LOG_Channel physics_log_channel;
	
	PHYS_Engine physics_engine;

	// ---

	LOG_Channel entity_log_channel;
	
	ENT_World world;
	ENT_EventQueue events;

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

internal void AppInitPhysics         (App *app);
internal void AppDestroyPhysics      (App *app);
internal void AppHotLoadPhysics      (App *app);
internal void AppHotUnloadPhysics    (App *app);

internal void AppInitEntity          (App *app);
internal void AppDestroyEntity       (App *app);
internal void AppHotLoadEntity       (App *app);
internal void AppHotUnloadEntity     (App *app);

internal void AppInitEditor          (App *app);
internal void AppDestroyEditor       (App *app);
internal void AppHotLoadEditor       (App *app);
internal void AppHotUnloadEditor     (App *app);

internal void AppRender(App *app, f32 dt, f32 elapsed, GFX_CmdBuffer *cmd);

__declspec(dllexport) App  *AppInit      (const OS_API *api);
__declspec(dllexport) void  AppDestroy   (App *app);
__declspec(dllexport) b32   AppTick      (App *app, const I_State *input);
__declspec(dllexport) void  AppHotLoad   (App *app, const OS_API *api);
__declspec(dllexport) void  AppHotUnload (App *app);

#endif // APP_H
