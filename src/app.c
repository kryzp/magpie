#include "assert.h"

#define STBI_ASSERT Assert
#define STBIW_ASSERT Assert

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

#include "arena.h"
#include "arena.c"

#include "graphics_device.h"
#include "assets.h"
#include "renderer.h"
#include "app.h"
#include "scratch.h"

#include "graphics_device.c"
#include "assets.c"
#include "renderer.c"

internal void
Init(Platform *platform)
{
	MemoryArena permanent_arena = MemoryArenaInit(platform->permanent_memory, platform->permanent_memory_size);
	
	app = MemoryArenaPush(&permanent_arena, sizeof(App));
	
	app->permanent_arena = permanent_arena;
	app->frame_arena = MemoryArenaInit(platform->frame_memory, platform->frame_memory_size);
	
	app->scratch_arenas[0] = MemoryArenaInit(platform->scratch_memory[0], platform->scratch_memory_size);
	app->scratch_arenas[1] = MemoryArenaInit(platform->scratch_memory[1], platform->scratch_memory_size);
	
	GraphicsDeviceInit(&app->permanent_arena, platform);
	RendererInit(&app->renderer, &app->permanent_arena);
}

internal void
Destroy(Platform *platform)
{
	RendererDestroy(&app->renderer);
	GraphicsDeviceDestroy();
}

internal void
Update(Platform *platform)
{
	MemoryArenaClear(&app->frame_arena);
	
	MemoryArenaClear(&app->scratch_arenas[0]);
	MemoryArenaClear(&app->scratch_arenas[1]);
	
	if(platform->input.kb_pressed[KeyboardKey_Escape])
	{
		platform->exit = 1;
	}
	
	f32 delta_time = 1.f / (f32)platform->target_fps;
	
	RendererBeginFrame(&app->renderer);
	{
	}
	RendererEndFrame(&app->renderer);
	
	/*
		CommandBuffer *cmd = BeginGraphicsPresent();
		{
			RenderContext context = {0};
			context.cmd = cmd;
			context.swapchain = GetGraphicsSwapchain();
			context.scene = &app->scene;
			context.camera = &app->camera;
			
			RendererRender(&app->renderer, &context);
		}
		EndGraphicsPresent(cmd);
		*/
}

internal void
FixedUpdate(Platform *platform)
{
}
