
// TODO: Move these out into their respective
//       backends.

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb/stb_image_write.h"

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <vma/vk_mem_alloc.h>

#define SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
#include "ext/spirv/spirv_reflect.h"
#include "ext/spirv/spirv_reflect.c"

#define MINIAUDIO_IMPLEMENTATION
#include "ext/ma/miniaudio.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// who decided to name these macros ffs
#undef min
#undef max
#undef near
#undef far

// ---

#include "core/core_inc.h"
#include "input/input_inc.h"
#include "os/os_inc.h"
#include "io/io_inc.h"
#include "chrono/chrono_inc.h"
#include "graphics/graphics_inc.h"
#include "audio/audio_inc.h"
#include "asset/asset_inc.h"
#include "animation/animation_inc.h"
#include "render/render_inc.h"
#include "entity/entity_inc.h"
#include "timeline/timeline_inc.h"
#include "dev/dev_inc.h"
#include "gamemode/gamemode_inc.h"
#include "cutscene/cutscene_inc.h"
#include "encounter/encounter_inc.h"

#include "camera_driver.h"
#include "app.h"

// ---

#include "core/core_inc.c"
#include "input/input_inc.c"
#include "os/os_inc.c"
#include "io/io_inc.c"
#include "chrono/chrono_inc.c"
#include "graphics/graphics_inc.c"
#include "audio/audio_inc.c"
#include "asset/asset_inc.c"
#include "animation/animation_inc.c"
#include "render/render_inc.c"
#include "entity/entity_inc.c"
#include "timeline/timeline_inc.c"
#include "dev/dev_inc.c"
#include "gamemode/gamemode_inc.c"
#include "cutscene/cutscene_inc.c"
#include "encounter/encounter_inc.c"

#include "camera_driver.c"


/* ==================================================
   AUDIO
   ================================================== */

internal void
AppInitAudio(App *app)
{
	app->audio_backend = AUD_BackendAllocAndSelect(&app->partitions[AppMemoryPartition_Audio]);
	app->audio_backend->Init();
	
	AUD_Init(&app->audio_system, &app->partitions[AppMemoryPartition_Audio], app->audio_backend);
}

internal void
AppDestroyAudio(App *app)
{
	AUD_Shutdown(&app->audio_system);
	
	app->audio_backend->Shutdown();
}

internal void
AppHotLoadAudio(App *app)
{
	AUD_BackendHotLoad(app->audio_backend);
}

internal void
AppHotUnloadAudio(App *app)
{
	AUD_BackendHotUnload(app->audio_backend);
}


/* ==================================================
   GRAPHICS
   ================================================== */

internal void
AppInitGraphics(App *app)
{
	GFX_DeviceInit(&app->graphics_device, &app->partitions[AppMemoryPartition_Graphics]);
	app->swapchain = GFX_DeviceSwapchainCreate(&app->graphics_device);

	GFX_ShaderCompilerInit(&app->shader_compiler);

  
	GFX_BufferAllocInfo ring_buffer_alloc_info = {0};
	ring_buffer_alloc_info.size = Megabytes(512);
	ring_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ring_buffer_alloc_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;

	app->frame_upload_ring_buffer = GFX_RingBufferAlloc(&app->graphics_device, &ring_buffer_alloc_info);

	
	GFX_BufferAllocInfo frame_buffer_alloc_info = {0};
	frame_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	frame_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	frame_buffer_alloc_info.size = sizeof(R_GPU_FrameData);

	app->frame_data_buffer = GFX_DeviceBufferAlloc(&app->graphics_device, &frame_buffer_alloc_info);

	
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f)), // Right.
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f)), // Left.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f), v3( 0.f,-1.f, 0.f)), // Up.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f,-1.f), v3( 0.f, 1.f, 0.f)), // Down.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 1.f, 0.f), v3( 0.f, 0.f, 1.f)), // Forward.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,-1.f, 0.f), v3( 0.f, 0.f, 1.f)), // Backwards.
	};

	m4 capture_projection_matrix = M4Perspective(90.f, 1.f, 0.1f, 10.f);

	for (u32 i = 0; i < 6; i++)
		capture_view_matrices[i] = M4MulM4(capture_projection_matrix, capture_view_matrices[i]);

	GFX_BufferAllocInfo cubemap_capture_buffer_alloc_info = {0};
	cubemap_capture_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	cubemap_capture_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	cubemap_capture_buffer_alloc_info.size = sizeof(capture_view_matrices);

	app->cubemap_capture_transform_buffer = GFX_DeviceBufferAlloc(&app->graphics_device, &cubemap_capture_buffer_alloc_info);

	GFX_BufferWrite(GFX_DeviceBufferFromKey(&app->graphics_device, app->cubemap_capture_transform_buffer),
					capture_view_matrices,
					sizeof(capture_view_matrices), 0);
}

internal void
AppDestroyGraphics(App *app)
{
	GFX_RingBufferDestroy(&app->frame_upload_ring_buffer, &app->graphics_device);

	GFX_DeviceBufferDestroy(&app->graphics_device, app->frame_data_buffer);
	GFX_DeviceBufferDestroy(&app->graphics_device, app->cubemap_capture_transform_buffer);

	GFX_ShaderCompilerShutdown(&app->shader_compiler);
	
	GFX_DeviceSwapchainDestroy(&app->graphics_device, &app->swapchain);
	GFX_DeviceDestroy(&app->graphics_device);
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
   ENTITY
   ================================================== */

internal void
AppInitEntity(App *app)
{
}

internal void
AppDestroyEntity(App *app)
{
}

internal void
AppHotLoadEntity(App *app)
{
}

internal void
AppHotUnloadEntity(App *app)
{
}


/* ==================================================
   MEMORY PARTITION
   ================================================== */

internal Arena *
PartitionMemory(Arena *out, Arena *memory, u32 count, const f32 *ratio)
{
	f32 total = 0.f;

	for (u32 i = 0; i < count; i++)
		total += ratio[i];

	AssertTrue(total > 0.f);
	
	Arena *partitions = ArenaPushArray(out, Arena, count);

	const u64 alignment_reserve = 4 * count; // mem arenas push at 4 byte alignment

	AssertTrue(memory->capacity - memory->used > alignment_reserve);
	
	const u64 left = memory->capacity - memory->used - alignment_reserve;

	u64 assigned = 0;
	
	for (u32 p = 0; p < count; p++)
	{
		u64 size = 0;

		if (p == count - 1)
			size = left - assigned; // remainder
		else
			size = (u64)((f64)left * (ratio[p] / total));

		partitions[p] = ArenaInitArena(memory, size);
		assigned += size;
	}
	
	return partitions;
}


/* ==================================================
   APP
   ================================================== */

internal void
AppInit_(App *app)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	AppInitGraphics(app);
	AppInitAudio(app);

	AST_Init(&app->assets, &app->partitions[AppMemoryPartition_Assets], &app->graphics_device, &app->shader_compiler, app->audio_backend);
	AST_Mount(&app->assets, String8Lit("assets"), String8Lit("res/"));

	app->graph_arena = ArenaInitArena(&app->partitions[AppMemoryPartition_Render], app->partitions[AppMemoryPartition_Render].capacity * 0.5f);
	app->scene_arena = ArenaInitArena(&app->partitions[AppMemoryPartition_Render], app->partitions[AppMemoryPartition_Render].capacity * 0.5f);
	
	R_GraphInit(&app->graph, &app->graph_arena);
	R_SceneInit(&app->scene, &app->scene_arena, &app->graphics_device);
	
	app->camera = R_CameraPerspective(v3x(0.f), v3(0.f, 1.f, 0.f), 90.f, 1280.f / 720.f, .1f, 100.f);
	
	AppInitEntity(app);

	GM_StackInit(&app->game_mode_stack);

	CameraDriverConfig camera_driver_cfg = {0};
	camera_driver_cfg.mode = CameraDriverMode_Unrestricted;
	
	app->camera_driver = CameraDriverInit(&camera_driver_cfg);
	
	CH_TimerStart(&app->elapsed_timer);
	CH_TimerStart(&app->delta_timer);
	CH_TimerStart(&app->hot_reload_timer);

	ScratchRelease(&scratch);
}

__declspec(dllexport) App *
AppInit(Arena *arena, const OS_API *api)
{
	osapi = api;
	
	App *app = ArenaPushArray(arena, App, 1);

	static f32 memory_ratios[AppMemoryPartition_COUNT] = {
#define Partition(name, ratio) [AppMemoryPartition_##name] = (f32)(ratio),
#include "partitions.inc"
#undef Partition
	};
	
	app->partitions = PartitionMemory(arena, arena, ArraySize(memory_ratios), memory_ratios);
	
	AppInit_(app);

	return app;
}

__declspec(dllexport) void
AppDestroy(App *app)
{
	GFX_DeviceWaitIdle(&app->graphics_device);

	AppDestroyEntity(app);
	AppDestroyAudio(app);

	R_SceneDestroy(&app->scene);
	R_GraphDestroy(&app->graph, &app->graphics_device);

	AST_Destroy(&app->assets);
	
	AppDestroyGraphics(app);
}

__declspec(dllexport) b32
AppTick(App *app, const I_State *input)
{
	if (I_KbPressed(input, I_KeyboardKey_Escape))
		return true;

	const f32 elapsed = CH_TimerElapsed(&app->elapsed_timer);
	const f32 dt = CH_TimerReset(&app->delta_timer);
	const f32 fixed_dt = 1.f / APP_TARGET_FPS;

	if (CH_TimerElapsed(&app->hot_reload_timer) >= APP_HOT_RELOAD_INTERVAL)
	{
		CH_TimerReset(&app->hot_reload_timer);
		AST_PollHotReloads(&app->assets);
	}

	AST_FlushUploads(&app->assets);

	GM_StackTick(&app->game_mode_stack, app, dt, input);

	ENT_WorldTickPreAnim(&app->world, &app->events, dt, input);

	// TODO: animation system

	ENT_WorldTickPostAnim(&app->world, &app->events, dt, input);

	app->delta_accumulator += MinValue(dt, fixed_dt);

	while (app->delta_accumulator >= fixed_dt)
	{
		// TODO: physics update here (using fixed_dt)
		
		app->delta_accumulator -= fixed_dt;
	}
	
	ENT_WorldTickPostPhysics(&app->world, &app->events, dt, input);

	ENT_EventDispatch(&app->events, &app->world);

	ENT_WorldFlush(&app->world);
	
	AUD_Listener listener = {0};
	listener.eye = app->camera.position;
	listener.forward = app->camera.forward;
	listener.up = v3(0.f, 0.f, 1.f);
	
	AUD_Tick(&app->audio_system, dt, listener);

	CameraDriverDrive(&app->camera_driver, &app->camera, input, dt);
	
	GFX_CmdBuffer cmd = GFX_DeviceBeginFrame(&app->graphics_device, &app->swapchain);
	{
		R_SceneDebug(&app->scene);
	
		R_SceneResources scene_resources = R_SceneRefreshTransientResources(&app->scene, &app->frame_upload_ring_buffer);

		R_TextureInfo swapchain_attachment_info = R_TextureInfoInit();
		swapchain_attachment_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	
		app->swapchain_src = R_GraphCreateTexture(&app->graph, &swapchain_attachment_info);
	
		AppRender(app, dt, elapsed, &cmd);

		R_Clear clear = R_ClearColour((f32)I_KbDown(input, I_KeyboardKey_Tab), 0.0f, 0.0f, 1.f);

		R_Pass *dummy = R_GraphAdd(&app->graph, String8Lit("dummy"), R_PassType_Graphics);
		R_PassWriteColour(dummy, app->swapchain_src, &clear);

		R_GraphSetBackbuffer(&app->graph, app->swapchain_src);
		R_GraphCompile(&app->graph, &app->graphics_device, &app->swapchain);
		R_GraphExecute(&app->graph, &app->graphics_device, &app->swapchain, &cmd, &app->scene, &app->camera, dt, elapsed);
		R_GraphPresentToSwapchain(&app->graph, &app->graphics_device, &app->swapchain, &cmd);
		R_GraphReset(&app->graph, &app->graphics_device);
	}
	GFX_DeviceEndFrame(&app->graphics_device, &app->swapchain, &cmd);
	
	return false;
}

__declspec(dllexport) void
AppHotLoad(App *app, const OS_API *api)
{
	osapi = api;
	
	AppHotLoadGraphics   (app);
	AppHotLoadAudio      (app);
}

__declspec(dllexport) void
AppHotUnload(App *app)
{	
	AppHotUnloadAudio      (app);
	AppHotUnloadGraphics   (app);
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

	GFX_BufferWrite(GFX_DeviceBufferFromKey(&app->graphics_device, app->frame_data_buffer),
					&frame_data, sizeof(frame_data), 0);

	R_Blackboard bb = {0};
}
