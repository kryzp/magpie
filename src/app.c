
/*
 * The Echo or The Answer
 * ---
 * At the willow tree grows the wallflower,
 * pale and cold,
 * from dust, alone.
 *
 * To the voices outside unresponsive,
 * blossoming only for the wind,
 * and to the touch, you are cold.
 *
 * Of no mother, nor child,
 * to bear your name,
 * to free your spirit.
 *
 * And I will meet you there,
 * and so it shall be,
 * together at last.
 *
 * But till that day may pass,
 * I have but one final question,
 * what is it, you fear most?
 */

// ---

// Todo: Move these out into their respective
//       backends.

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb/stb_image_write.h"

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <vma/vk_mem_alloc.h>

#include "ext/slang_compiler.h"

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
#include "log/log_inc.h"
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
#include "log/log_inc.c"
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
   LOG
   ================================================== */

internal void
AppInitLog(App *app)
{
	LOG_InitAndSelect(&app->logger, String8Lit("log_output.txt"));

	app->log_channel = LOG_OpenChannel(String8Lit("APP"));
}

internal void
AppDestroyLog(App *app)
{
	LOG_Shutdown();
}

internal void
AppHotLoadLog(App *app)
{
	LOG_HotLoad(&app->logger);
}

internal void
AppHotUnloadLog(App *app)
{
	LOG_HotUnload();
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
	
	GFX_DeviceBufferWrite(&app->graphics_device,
						  app->cubemap_capture_transform_buffer,
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
   ASSETS
   ================================================== */

internal void
AppInitAssets(App *app)
{
	AST_Init(&app->assets, &app->partitions[AppMemoryPartition_Assets], &app->graphics_device, &app->shader_compiler, app->audio_backend);
	AST_Mount(&app->assets, String8Lit("assets"), String8Lit("res/"));
}

internal void
AppDestroyAssets(App *app)
{
	AST_Destroy(&app->assets);
}

internal void
AppHotLoadAssets(App *app)
{
}

internal void
AppHotUnloadAssets(App *app)
{
}


/* ==================================================
   RENDER
   ================================================== */

internal void
AppInitRender(App *app)
{
	app->render_log_channel = LOG_OpenChannel(String8Lit("RENDER"));
	
	u64 render_third_size = ArenaSafePartitionSize(&app->partitions[AppMemoryPartition_Render], 3, 8);

	app->graph_arena      = ArenaInitArena(&app->partitions[AppMemoryPartition_Render], render_third_size, 8);
	app->scene_arena      = ArenaInitArena(&app->partitions[AppMemoryPartition_Render], render_third_size, 8);
	app->pass_frame_arena = ArenaInitArena(&app->partitions[AppMemoryPartition_Render], render_third_size, 8);
	
	R_GraphInit(&app->graph, &app->graph_arena, &app->graphics_device, app->render_log_channel);
	R_SceneInit(&app->scene, &app->scene_arena, &app->graphics_device, app->render_log_channel);
	
	app->camera = R_CameraPerspective(v3x(0.f), v3(0.f, 1.f, 0.f), 90.f, 1280.f / 720.f, .1f, 100.f);

	AST_Handle hdr_texture_handle = AST_Require(&app->assets, String8Lit("assets://environment_map_1.hdr"), AST_Type_Texture);
	
	AST_WaitForAsync(&app->assets);
	AST_FlushUploads(&app->assets);

	GFX_TextureKey hdr_texture_gfx = AST_Get(&app->assets, hdr_texture_handle, AST_Type_Texture)->texture.key;
	
	// Irradiance.
	{
		R_IBLPassIrradianceFnData *data = ArenaPushArray(&app->pass_frame_arena, R_IBLPassIrradianceFnData, 1);
	
		data->shader             = GFX_ShaderKeyNull();
		data->sampler            = GFX_SamplerKeyNull();
		data->env_view           = GFX_TextureViewKeyNull();
		data->capture_transforms = GFX_BufferKeyNull();
		data->cube_mesh          = NULL;
		
		//R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("Irradiance"), R_PassType_Graphics);

		//R_PassSetRecord           (pass, R_IBLPassIrradianceFn, data);
		//R_PassSetMultiViewMask    (pass, 0b111111);
		//R_PassWriteColour         (pass, R_GraphImportTexture(&app->graph, /* output irradiance cubemap */), NULL);
	}
	
	// Prefilter.
	{
		R_IBLPassPrefilterFnData *data = ArenaPushArray(&app->pass_frame_arena, R_IBLPassPrefilterFnData, 1);

		// TODO: add prefilter passes.
	}
	
	// TODO: execute & clear render graph
}

internal void
AppDestroyRender(App *app)
{
	R_SceneDestroy(&app->scene);
	R_GraphDestroy(&app->graph);
}

internal void
AppHotLoadRender(App *app)
{
}

internal void
AppHotUnloadRender(App *app)
{
}


/* ==================================================
   ENTITY
   ================================================== */

internal void
AppInitEntity(App *app)
{
	ENT_WorldInit(&app->world, &app->partitions[AppMemoryPartition_Entity]);
	ENT_EventQueueInit(&app->events);
}

internal void
AppDestroyEntity(App *app)
{
	ENT_WorldDestroy(&app->world);
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
   APP
   ================================================== */

internal void
AppInit_(App *app)
{
	AppInitLog      (app);
	AppInitGraphics (app);
	AppInitAudio    (app);
	AppInitAssets   (app);
	AppInitRender   (app);
	AppInitEntity   (app);

	// ---
	
	GM_StackInit(&app->game_mode_stack);

	CameraDriverConfig camera_driver_cfg = {0};
	camera_driver_cfg.mode = CameraDriverMode_Unrestricted;
	
	app->camera_driver = CameraDriverInit(&camera_driver_cfg);
	
	CH_TimerStart(&app->elapsed_timer);
	CH_TimerStart(&app->delta_timer);
	CH_TimerStart(&app->hot_reload_timer);
}

__declspec(dllexport) App *
AppInit(Arena *arena, const OS_API *api)
{
	osapi = api;
	
	App *app = ArenaPushArray(arena, App, 1);

	static f64 memory_ratios[AppMemoryPartition_COUNT] = {
#define Partition(name, ratio) (f32)(ratio),
#include "partitions.inc"
#undef Partition
	};
	
	f64 total_ratio = 0.0;

	for (u32 i = 0; i < AppMemoryPartition_COUNT; i++)
		total_ratio += memory_ratios[i];

	AssertTrue(total_ratio > 0.0);

	static u64 memory_sizes[AppMemoryPartition_COUNT] = {0};
	
	u64 left = arena->capacity - arena->used
			   - (AppMemoryPartition_COUNT * 8); // correct for alignment

	u64 allocated = 0;

	for (u32 i = 0; i < AppMemoryPartition_COUNT; i++)
	{
		memory_sizes[i] = (u64)((memory_ratios[i] / total_ratio) * left);
		allocated += memory_sizes[i];
	}

	u64 remainder = left - allocated;

	for (u32 i = 0; remainder > 0; i++)
	{
		memory_sizes[i % AppMemoryPartition_COUNT]++;
		remainder--;
	}

	for (u32 i = 0; i < AppMemoryPartition_COUNT; i++)
		app->partitions[i] = ArenaInitArena(arena, memory_sizes[i], 8);
	
	AppInit_(app);

	DebugLogI(app->log_channel, "Initialized.");
	
	return app;
}

__declspec(dllexport) void
AppDestroy(App *app)
{
	GFX_DeviceWaitIdle(&app->graphics_device);

	DebugLogI(app->log_channel, "Shutting Down...");
	
	AppDestroyEntity   (app);
	AppDestroyRender   (app);
	AppDestroyAssets   (app);
	AppDestroyAudio    (app);
	AppDestroyGraphics (app);
	AppDestroyLog      (app);
}

__declspec(dllexport) b32
AppTick(App *app, const I_State *input)
{
	if (I_KbPressed(input, I_KeyboardKey_Escape))
		return true;

	const f32 max_frame_time = 0.2f;
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

	f32 clamped_delta = dt;

	if (clamped_delta > max_frame_time)
	{
		clamped_delta = max_frame_time;
		DebugLogW(app->log_channel, "Had to clamp delta - is there lag?");
	}
	
	app->delta_accumulator += clamped_delta;

	while (app->delta_accumulator >= fixed_dt)
	{
		// TODO: physics update here (using fixed_dt)
		
		app->delta_accumulator -= fixed_dt;
	}
	
	ENT_WorldTickPostPhysics(&app->world, &app->events, dt, input);

	ENT_EventDispatch(&app->events, &app->world);

	ENT_WorldFlush(&app->world);
	
	AUD_Listener listener = {0};
	listener.position = app->camera.position;
	listener.direction = app->camera.forward;
	
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

		R_GraphSetBackbuffer(&app->graph, app->swapchain_src);
		R_GraphCompile(&app->graph, &app->swapchain);
		R_GraphExecute(&app->graph, &app->swapchain, &cmd, &app->scene, &app->camera, dt, elapsed);
		R_GraphPresentToSwapchain(&app->graph, &app->swapchain, &cmd);
		R_GraphReset(&app->graph);
	}
	GFX_DeviceEndFrame(&app->graphics_device, &app->swapchain, &cmd);
	
	ArenaClear(&app->pass_frame_arena);
	
	return false;
}

__declspec(dllexport) void
AppHotLoad(App *app, const OS_API *api)
{
	osapi = api;
	
	AppHotLoadLog        (app);
	AppHotLoadGraphics   (app);
	AppHotLoadAudio      (app);
	AppHotLoadAssets     (app);
	AppHotLoadRender     (app);
	AppHotLoadEntity     (app);
}

__declspec(dllexport) void
AppHotUnload(App *app)
{
	AppHotUnloadEntity     (app);
	AppHotUnloadRender     (app);
	AppHotUnloadAssets     (app);
	AppHotUnloadAudio      (app);
	AppHotUnloadGraphics   (app);
	AppHotUnloadLog        (app);
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

	GFX_DeviceBufferWrite(&app->graphics_device,
						  app->frame_data_buffer,
						  &frame_data, sizeof(frame_data), 0);

	R_Blackboard bb = {0};
		
	R_Clear clear = R_ClearColour(CosF(elapsed), SinF(elapsed), CosF(elapsed) * SinF(elapsed), 1.f);
	R_Pass *dummy = R_GraphAdd(&app->graph, String8Lit("dummy"), R_PassType_Graphics);
	R_PassWriteColour(dummy, app->swapchain_src, &clear);
}
