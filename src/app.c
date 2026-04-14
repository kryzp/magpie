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
#include "cutscene/cutscene_inc.h"

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

internal void
CameraDriverDrive(CameraDriver *driver, R_Camera *camera)
{
	// TODO
}

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
AppTickAudio(App *app, f32 dt)
{
	AUD_Listener listener = {0};
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
}

internal void
AppDestroyGraphics(App *app)
{
}

internal void
AppTickGraphics(App *app, f32 dt)
{
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

	const f32 dt = 0.1f;

	AppTickAudio(app, dt);
	AppTickGraphics(app, dt);
	
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
