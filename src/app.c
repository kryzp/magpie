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

global App *app_ptr = NULL;

internal void
CameraDriverDrive(CameraDriver *driver, R_Camera *camera)
{
	// TODO
}

__declspec(dllexport) void
AppInit(const OS_BootstrapData *data)
{
	Arena tmp = ArenaInitMemory(data->memory, data->memory_size);

	app_ptr = ArenaPushArray(&tmp, App, 1);

	app_ptr->permanent_arena = tmp;
	app_ptr->frame_arena = ArenaInitArena(&app_ptr->permanent_arena, Megabytes(512));

	app_ptr->osapi = data->api;
}

__declspec(dllexport) void
AppDestroy(void)
{
}

__declspec(dllexport) b32
AppTick(const I_InputSt *input)
{
	ArenaClear(&app_ptr->frame_arena);

	if (I_KbPressed(input, I_KeyboardKey_Escape))
		return true;
	
	return false;
}

__declspec(dllexport) void
AppHotLoad(const OS_BootstrapData *data)
{
	app_ptr = data->memory;
	osapi = app_ptr->osapi;
}

__declspec(dllexport) void
AppHotUnload(void)
{
}
