#define VOLK_IMPLEMENTATION
#include "ext/volk.h"

#include "ext/vk_mem_alloc.h"

#define SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
#include "ext/spirv_reflect.c"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "abstraction_layer.h"

#define STBI_ASSERT Assert
#define STBIW_ASSERT Assert

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

#include "program_constants.h"
#include "platform.h"

#include "arena.h"
#include "arena.c"

#include "timer.h"
#include "hash_table.h"
#include "graphics_device.h"
#include "model.h"
#include "assets.h"
#include "scene.h"
#include "renderer.h"
#include "vertex_formats.h"
#include "core.h"
#include "scratch.h"

#include "timer.c"
#include "scratch.c"
#include "hash_table.c"
#include "graphics_device.c"
#include "vertex_formats.c"
#include "model.c"
#include "assets.c"
#include "renderer.c"
#include "scene.c"

internal void
CoreResetGlobals(Platform *p)
{
	platform = p;
	
	core = platform->permanent_memory;
	
	graphics_device = &core->graphics_device;
	vertex_formats = &core->vertex_formats;
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
	
	AssetsInit(&core->assets, &core->permanent_arena);
	
	GraphicsDeviceInit(platform, &core->permanent_arena);
	VertexFormatsInit(&core->vertex_formats, &core->permanent_arena);
	RendererInit(&core->renderer, &core->assets, &core->permanent_arena);
	
	u32 damaged_helmet_model_asset_handle = AssetsLoadModel(&core->assets,
															str8("res/DamagedHelmet/DamagedHelmet.gltf"));
	
	core->damaged_helmet_model = core->assets.models[damaged_helmet_model_asset_handle].model;
	
	core->main_camera = CameraInitPerspective(v3(0.f, 0.f, 0.f),
											  v3(0.f, 1.f, 0.f),
											  100.f, 1280.f/720.f,
											  .1f, 10.f);
	
	core->starting_ticks = platform->GetPerformanceCounter();
}

void
CoreUpdate(Platform *p)
{
	MemoryArenaClear(&core->frame_arena);
	MemoryArenaClear(&core->scratch_arenas[0]);
	MemoryArenaClear(&core->scratch_arenas[1]);
	
	if(platform->kb_pressed[KeyboardKey_Escape])
	{
		DebugLog("Quitting...");
		platform->exit = 1;
	}
	
	core->main_camera.position = v3(0.f, -2.f, 0.f);
	core->main_camera.forward = v3(0.f, 1.f, 0.f);
	
	CameraRecompute(&core->main_camera);
	
	RendererSetCamera(&core->renderer, &core->main_camera);
	RendererBeginFrame(&core->renderer);
	{
		f32 t = GetTotalElapsedSecondsF();
		
		RenderCall call = {0};
		call.mesh = &core->damaged_helmet_model.sub_models[0].mesh;
		call.material = &core->damaged_helmet_model.sub_models[0].material;
		call.transform = M4Transform(v3(0.f, 0.f, .4f*SinF(t)),
									 QuatInitEuler(0.f, t, 0.f),
									 v3(1.f, 1.f, 1.f),
									 v3(0.f, 0.f, 0.f));
		
		RendererPushCall(&core->renderer, &call);
		
		Light light = {0};
		light.type = LightType_Point;
		light.position = v3(0.f, 2.f, 2.f);
		light.colour = v3(1.f, 1.f, 1.f);
		light.intensity = 1.f + SinF(t)*.5f;
		light.falloff = 1.f;
		
		RendererPushLight(&core->renderer, &light);
	}
	RendererEndFrame(&core->renderer);
}

void
CoreDestroy(Platform *p)
{
	AssetsDestroy(&core->assets);
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
