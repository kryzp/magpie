#include "core/core_inc.h"
#include "os/os_inc.h"
#include "input/input_inc.h"
#include "io/io_inc.h"
#include "chrono/chrono_inc.h"
#include "dev/dev_inc.h"
#include "graphics/graphics_inc.h"
#include "asset/asset_inc.h"
#include "render/render_inc.h"
#include "audio/audio_inc.h"
#include "animation/animation_inc.h"
#include "timeline/timeline_inc.h"
#include "cutscene/cutscene_inc.h"

#include "game_mode.h"
#include "camera_driver.h"
#include "app.h"

#include "core/core_inc.c"
#include "os/os_inc.c"
#include "input/input_inc.c"
#include "io/io_inc.c"
#include "chrono/chrono_inc.c"
#include "dev/dev_inc.c"
#include "graphics/graphics_inc.c"
#include "asset/asset_inc.c"
#include "render/render_inc.c"
#include "audio/audio_inc.c"
#include "animation/animation_inc.c"
#include "cutscene/cutscene_inc.c"

#include "camera_driver.c"


/* ==================================================
   AUDIO
   ================================================== */

internal void
AppInitAudio(App *app)
{
	app->audio_backend = AUD_BackendAllocAndSelect(app->permanent_arena);
	app->audio_backend->Init();
	
	AUD_Init(&app->audio_system, app->permanent_arena, app->audio_backend);
}

internal void
AppDestroyAudio(App *app)
{
	AUD_Shutdown(&app->audio_system);
	
	app->audio_backend->Shutdown();
}

internal void
AppTickAudio(App *app, f32 dt, f32 elapsed)
{
	AUD_Listener listener = {0};
	listener.eye     = app->camera.position;
	listener.forward = app->camera.forward;
	listener.up      = v3(0.f, 0.f, 1.f);
	
	AUD_Tick(&app->audio_system, dt, listener);
}

internal void
AppHotLoadAudio(App *app)
{
	AUD_BackendSelect(app->audio_backend);
}

internal void
AppHotUnloadAudio(App *app)
{
}

/* ==================================================
   GRAPHICS
   ================================================== */

internal void
AppInitGraphics(App *app)
{
	GFX_DeviceInit(&app->graphics_device);

	app->swapchain = GFX_DeviceSwapchainCreate(&app->graphics_device);

	// TODO
}

internal void
AppDestroyGraphics(App *app)
{
}

internal void
AppTickGraphics(App *app, f32 dt, f32 elapsed)
{	
	GFX_CmdBuffer cmd = GFX_DeviceBeginFrame(&app->graphics_device, &app->swapchain);

	R_SceneResources scene_resources = R_SceneRefreshTransientResources(&app->scene, &app->frame_upload_ring_buffer);

	R_TextureInfo swapchain_attachment_info = R_TextureInfoInit();
	swapchain_attachment_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	
	app->swapchain_src = R_GraphCreateTexture(swapchain_attachment_info);
	
	AppRender(app, dt, elapsed_time, &cmd);

	R_GraphCompile(&app->graph, &app->graphics_device, &app->swapchain);
	R_GraphExecute(&app->graph, &app->graphics_device, &app->Swapchain, &cmd, &app->scene, &app->camera, dt, elapsed_time);
	R_GraphReset(&app->graph, &app->graphics_device);
	
	GFX_DeviceEndFrame(&app->graphics_device, &app->swapchain, &cmd);
}

internal void
AppHotLoadGraphics(App *app)
{
	GFX_DeviceHotLoad(&app->graphics_device);
}

internal void
AppHotUnloadGraphics(App *app)
{
	GFX_DeviceHotUnload(&app->graphics_device);
}


/* ==================================================
   APP
   ================================================== */

__declspec(dllexport) void *
AppInit(Arena *arena, const OS_API *api)
{
	osapi = api;
	
	App *app = ArenaPushArray(arena, App, 1);
	
	app->permanent_arena = arena;
	app->frame_arena = ArenaInitArena(app->permanent_arena, Megabytes(512));

	AppInitAudio(app);
	AppInitGraphics(app);

	return app;
}

__declspec(dllexport) void
AppDestroy(void *ctx)
{
	App *app = ctx;
	
	AppDestroyGraphics(app);
	AppDestroyAudio(app);
}

__declspec(dllexport) b32
AppTick(void *ctx, const I_InputSt *input)
{
	App *app = ctx;
	
	ArenaClear(&app->frame_arena);

	if (I_KbPressed(input, I_KeyboardKey_Escape))
		return true;

	const f32 elapsed_time = CH_TimerElapsed(&app->global_timer);
	const f32 dt = CH_TimerReset(&app->delta_timer);
	const f32 fixed_dt = 1.f / APP_TARGET_FPS;

	if (CH_TimerElapsed(&app->hot_reload_timer) >= APP_HOT_RELOAD_INTERVAL)
	{
		CH_TimerReset(&app->hot_reload_timer);
		AST_PollHotReloads(&app->assets);
	}

	AST_FlushUploads(&app->assets);

	AppUpdate(app, dt, elapsed_time, input);

	app->delta_accumulator += MinValue(dt, fixed_dt);

	while (app->delta_accumulator >= fixed_dt)
	{
		AppFixedUpdate(app, fixed_dt, elapsed_time, input);
		app->delta_accumulator -= fixed_dt;
	}

	AppTickAudio(app, dt, elapsed_time);
	AppTickGraphics(app, dt, elapsed_time);
	
	return false;
}

__declspec(dllexport) void
AppHotLoad(void *ctx, const OS_API *api)
{
	osapi = api;
	
	App *app = ctx;

	AppHotLoadGraphics(app);
	AppHotLoadAudio(app);
}

__declspec(dllexport) void
AppHotUnload(void *ctx)
{
	App *app = ctx;
	
	AppHotUnloadAudio(app);
	AppHotUnloadGraphics(app);
}


/* ==================================================
   APP BEHAVIOUR
   ================================================== */

internal void
AppUpdate(App *app, f32 dt, f32 elapsed, const I_InputSt *input)
{
	R_SceneDebug(&app->scene);
}

internal void
AppFixedUpdate(App *app, f32 dt, f32 elapsed, const I_InputSt *input)
{
}

internal void
AppRender(App *app, f32 dt, f32 elapsed, GFX_CmdBuffer *cmd)
{
	R_CameraRecompute(&app->camera);
	
	R_GPU_FrameData frame_data = {0};
	frame_data.view = app->camera.view;
	frame_data.proj = app->camera.proj;
	frame_data.view_proj = M4MulM4(app->camera.proj, app->camera.view);
	frame_data.view_proj_no_translation = M4RemoveTranslation(frame_data.view_proj);
	frame_data.inv_view = M4Inverse(app->camera.view);
	frame_data.inv_proj = M4Inverse(app->camera.proj);
	frame_data.camera_position = app->camera.position;
	frame_data.window_resolution = v2(1280.f, 720.f);
	frame_data.time = elapsed;

	GFX_BufferWrite(&app->frame_data_buffer, &frame_data, sizeof(frame_data), 0);

	R_Blackboard bb = {0};
}
