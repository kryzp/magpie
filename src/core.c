#define VOLK_IMPLEMENTATION
#include "ext/volk.h"
#include "ext/vk_mem_alloc.h"

#include "abstraction_layer.h"

#define STBI_ASSERT Assert
#define STBIW_ASSERT Assert

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

#include "program_options.h"
#include "platform.h"

#include "arena.h"
#include "arena.c"

#include "hash_table.h"
#include "graphics_device.h"
#include "assets.h"
#include "renderer.h"
#include "core.h"
#include "scratch.h"

#include "graphics_device.c"
#include "assets.c"
#include "renderer.c"

internal void
CoreResetGlobals(Platform *p)
{
	platform = p;
	core = platform->permanent_memory;
	graphics_device = &core->graphics_device;
}

void
CoreInit(Platform *p)
{
	CoreResetGlobals(p);
	
	MemoryArena permanent_arena = MemoryArenaInit(platform->permanent_memory, platform->permanent_memory_size);
	
	core = MemoryArenaPush(&permanent_arena, sizeof(Core));
	
	core->permanent_arena = permanent_arena;
	core->frame_arena = MemoryArenaInit(platform->transient_memory, platform->transient_memory_size);
	
	core->scratch_arenas[0] = MemoryArenaInit(platform->scratch_memory[0], platform->scratch_memory_size);
	core->scratch_arenas[1] = MemoryArenaInit(platform->scratch_memory[1], platform->scratch_memory_size);
	
	GraphicsDeviceInit(platform, &core->permanent_arena);
	RendererInit(&core->renderer, &core->permanent_arena);
}

void
CoreUpdate(Platform *p)
{
	MemoryArenaClear(&core->frame_arena);
	MemoryArenaClear(&core->scratch_arenas[0]);
	MemoryArenaClear(&core->scratch_arenas[1]);
	
	if(platform->input.kb_pressed[KeyboardKey_Escape])
	{
		platform->exit = 1;
	}
	
	//f32 delta_time = 1.f / (f32)platform->target_fps;
	
	RendererBeginFrame(&core->renderer);
	{
		//RendererPushModel(&core->renderer, &cube_model);
		
		//printf("Hello, World!\n");
	}
	RendererEndFrame(&core->renderer);
	
	/*
		CommandBuffer *cmd = BeginGraphicsPresent();
		{
			RenderContext context = {0};
			context.cmd = cmd;
			context.swapchain = GetGraphicsSwapchain();
			context.scene = &core->scene;
			context.camera = &core->camera;
			
			RendererRender(&core->renderer, &context);
		}
		EndGraphicsPresent(cmd);
		*/
}

void
CoreDestroy(Platform *p)
{
	RendererDestroy(&core->renderer);
	GraphicsDeviceDestroy();
}

void
CoreBeforeHotReload(Platform *p)
{
	GraphicsDeviceBeforeHotReload();
}

void
CoreAfterHotReload(Platform *p)
{
	CoreResetGlobals(p);
	GraphicsDeviceAfterHotReload();
}
