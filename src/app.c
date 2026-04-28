
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
#include "editor/editor_inc.h"

#include "app.h"

#define DebugLogEx(level, channel, ...) osapi->Log((level), (channel), __FILE__, __LINE__, __func__, __VA_ARGS__)

#define DebugLogT(channel, ...) DebugLogEx(LOG_Level_Trace, (channel), __VA_ARGS__)
#define DebugLogD(channel, ...) DebugLogEx(LOG_Level_Debug, (channel), __VA_ARGS__)
#define DebugLogI(channel, ...) DebugLogEx(LOG_Level_Info,  (channel), __VA_ARGS__)
#define DebugLogW(channel, ...) DebugLogEx(LOG_Level_Warn,  (channel), __VA_ARGS__)
#define DebugLogE(channel, ...) DebugLogEx(LOG_Level_Error, (channel), __VA_ARGS__)
#define DebugLogB(channel, ...) DebugLogEx(LOG_Level_Break, (channel), __VA_ARGS__)

// TODO: Kinda hacky, not a fan of this!!
#define DebugPrintT(...) DebugLogT(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintD(...) DebugLogD(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintI(...) DebugLogI(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintW(...) DebugLogW(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintE(...) DebugLogE(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintB(...) DebugLogB(LOG_ChannelNull(), __VA_ARGS__)

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
#include "editor/editor_inc.c"


/* ==================================================
   GRAPHICS
   ================================================== */

internal void
AppInitGraphics(App *app)
{
	app->graphics_log_channel = osapi->LogOpenChannel(String8Lit("GRAPHICS"));

	
	GFX_DeviceInit(&app->graphics_device, &app->graphics_arena, app->graphics_log_channel);


	app->swapchain = GFX_DeviceSwapchainCreate(&app->graphics_device);

	
	app->shader_compiler_log_channel = osapi->LogOpenChannel(String8Lit("SLANG"));
	GFX_ShaderCompilerInit(&app->shader_compiler, app->shader_compiler_log_channel);

  
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

	app->linear_sampler = GFX_DeviceSamplerCreateF(&app->graphics_device, VK_FILTER_LINEAR);
}

internal void
AppDestroyGraphics(App *app)
{
	GFX_DeviceSamplerDestroy(&app->graphics_device, app->linear_sampler);
	
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
	/*
	app->audio_log_channel = osapi->LogOpenChannel(String8Lit("AUDIO"));
	
	app->audio_backend = AUD_BackendAllocAndSelect(&app->audio_arena);
	app->audio_backend->Init();
	
	AUD_Init(&app->audio_system,
			 &app->audio_arena,
			 app->audio_log_channel,
			 app->audio_backend);
	*/
}

internal void
AppDestroyAudio(App *app)
{
	//AUD_Shutdown(&app->audio_system);
	
	//app->audio_backend->Shutdown();
}

internal void
AppHotLoadAudio(App *app)
{
	//AUD_BackendHotLoad(app->audio_backend);
}

internal void
AppHotUnloadAudio(App *app)
{
	//AUD_BackendHotUnload(app->audio_backend);
}


/* ==================================================
   ASSETS
   ================================================== */

internal void
AppInitAssets(App *app)
{
	app->asset_log_channel = osapi->LogOpenChannel(String8Lit("ASSETS"));
	
	AST_Init(&app->assets,
			 &app->asset_arena,
			 app->asset_log_channel,
			 &app->graphics_device,
			 &app->shader_compiler,
			 app->audio_backend);
	
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
AppInitRenderCreateSkyboxMesh(App *app)
{
	static v3 vertices[] = {
		{ -1.f,  1.f,  1.f },
		{ -1.f,  1.f, -1.f },
		{  1.f,  1.f, -1.f },
		{  1.f,  1.f,  1.f },
		{ -1.f, -1.f,  1.f },
		{ -1.f, -1.f, -1.f },
		{  1.f, -1.f, -1.f },
		{  1.f, -1.f,  1.f }
	};

	static u16 indices[] = {
		1, 2, 0,
		3, 0, 2,

		6, 5, 7,
		4, 7, 5,

		5, 1, 4,
		0, 4, 1,

		2, 6, 3,
		7, 3, 6,

		5, 6, 1,
		2, 1, 6,

		0, 3, 4,
		7, 4, 3
	};

	R_MeshAlloc(&app->skybox_mesh, &app->graphics_device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	GFX_BufferKey staging_buffer = GFX_DeviceStageAlloc(&app->graphics_device, R_MeshVertexBufferSize(&app->skybox_mesh) + R_MeshIndexBufferSize(&app->skybox_mesh));

	R_MeshWriteToStage(&app->skybox_mesh, &app->graphics_device,
					   staging_buffer, 0,
					   vertices, indices);

	GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(&app->graphics_device);
	
	R_MeshUpload(&app->skybox_mesh, &cmd,
				 staging_buffer, 0);

	GFX_DeviceSubmitImEnd(&app->graphics_device, &cmd);

	GFX_DeviceBufferDestroy(&app->graphics_device, staging_buffer);
}

internal void
AppInitRender(App *app)
{
	app->render_log_channel = osapi->LogOpenChannel(String8Lit("RENDER"));
	
	u64 render_quarter_size = ArenaSafePartitionSize(&app->render_arena, 4, 8);

	app->graph_arena      = ArenaInitArena(&app->render_arena, render_quarter_size, 8);
	app->scene_arena      = ArenaInitArena(&app->render_arena, render_quarter_size, 8);
	app->pass_frame_arena = ArenaInitArena(&app->render_arena, render_quarter_size, 8);
	app->debug_draw_arena = ArenaInitArena(&app->render_arena, render_quarter_size, 8);
	
	R_GraphInit(&app->graph, &app->graph_arena, &app->graphics_device, app->render_log_channel);
	R_SceneInit(&app->scene, &app->scene_arena, &app->graphics_device, app->render_log_channel);

	AppInitRenderCreateSkyboxMesh(app);

	app->environment_cubemap = GFX_DeviceTextureAllocCubemap(&app->graphics_device, 512, VK_FORMAT_R32G32B32A32_SFLOAT, 8);
	app->irradiance_cubemap  = GFX_DeviceTextureAllocCubemap(&app->graphics_device,  32, VK_FORMAT_R32G32B32A32_SFLOAT, 1);
	app->prefilter_cubemap   = GFX_DeviceTextureAllocCubemap(&app->graphics_device, 128, VK_FORMAT_R32G32B32A32_SFLOAT, 5);

	R_CullingInit          (&app->culling,           &app->assets);
	R_ShadowRendererInit   (&app->shadow_renderer,   &app->graphics_device, &app->assets);
	R_DeferredRendererInit (&app->deferred_renderer, &app->graphics_device, &app->assets);
	R_DebugRendererInit    (&app->debug_renderer,    &app->debug_draw_arena, &app->graphics_device, &app->assets);

	AST_Handle hdr_to_env_shader_handle = AST_RequireNow(&app->assets, String8Lit("assets://shaders/passes/hdr_to_environment_cubemap.slang"), AST_Type_Shader);
	AST_Handle irradiance_shader_handle = AST_RequireNow(&app->assets, String8Lit("assets://shaders/passes/irradiance_convolution.slang"),     AST_Type_Shader);

	AST_Handle hdr_texture_handle = AST_RequireNow(&app->assets, String8Lit("assets://environment_map_1.hdr"), AST_Type_Texture);
	
	AST_Handle model_handle = AST_RequireNow(&app->assets, String8Lit("assets://models/Sponza/glTF/Sponza.gltf"), AST_Type_Model);

	ScratchArena scratch = ScratchBegin(NULL, 0);

	R_SceneRegisterModelReceipt model_receipt = R_SceneRegisterModel(&app->scene, scratch.arena, &app->assets, model_handle, -1u);

	for (u32 i = 0; i < model_receipt.entry_count; i++)
	{
		R_SceneModelEntry *entry = &model_receipt.entries[i];

		R_Object obj = {0};
		obj.transform     = entry->transform;
		obj.sphere_bounds = entry->sphere_bounds;
		obj.mesh          = entry->mesh;
		obj.material      = entry->material;
		
		R_SceneObjectCreate(&app->scene, &obj);
	}
	
	ScratchRelease(&scratch);

	R_Light light = {0};
	light.type = R_LightType_Point;
	light.position = v3(0.f, 0.f, 1.f);
	light.direction = v3x(0.f);
	light.colour = v3(1.f, 1.f, 1.f);
	light.intensity = 3.f;
	light.falloff = 1.f;
	light.casts_shadows = true;
	light.shadow_near = 0.1f;
	light.shadow_far = 10.f;
	
	R_SceneLightCreate(&app->scene, &light);
	
	GFX_ShaderKey hdr_to_env_shader      = AST_Get(&app->assets, hdr_to_env_shader_handle, AST_Type_Shader)->shader.key;
	GFX_ShaderKey irradiance_pass_shader = AST_Get(&app->assets, irradiance_shader_handle, AST_Type_Shader)->shader.key;
	
	GFX_TextureKey hdr_texture_gfx = AST_Get(&app->assets, hdr_texture_handle, AST_Type_Texture)->texture.key;

	// Generate Environment Cubemap.
	{
		R_HdrToEnvPassData *data = ArenaPushArray(&app->pass_frame_arena, R_HdrToEnvPassData, 1);
		data->shader             = hdr_to_env_shader;
		data->sampler            = app->linear_sampler;
		data->hdr_view           = GFX_DeviceTextureViewAuto(&app->graphics_device, hdr_texture_gfx);
		data->capture_transforms = app->cubemap_capture_transform_buffer;
		data->skybox_mesh        = &app->skybox_mesh;
		
		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("HDR -> Environment Map"), R_PassType_Graphics);
		R_PassSetRecord        (pass, R_HdrToEnvPassFn, data);
		R_PassSetMultiViewMask (pass, 0b111111);
		R_PassWriteColour      (pass, R_GraphImportTexture(&app->graph, app->environment_cubemap), NULL);

		R_GenerateMipsPassData *mips_data = ArenaPushArray(&app->pass_frame_arena, R_GenerateMipsPassData, 1);
		mips_data->texture = app->environment_cubemap;
		
		R_Pass *pass_mipmaps = R_GraphAdd(&app->graph, String8Lit("Environment Map Mipmapping"), R_PassType_Transfer);
		R_PassSetRecord      (pass_mipmaps, R_GenerateMipsPassFn, mips_data);
		R_PassBlitTextureDst (pass_mipmaps, R_GraphImportTexture(&app->graph, app->environment_cubemap));
	}
	
	// Irradiance.
	{
		R_IBLPassIrradianceData *data = ArenaPushArray(&app->pass_frame_arena, R_IBLPassIrradianceData, 1);
		data->shader             = irradiance_pass_shader;
		data->sampler            = app->linear_sampler;
		data->env_view           = GFX_DeviceTextureViewAuto(&app->graphics_device, app->environment_cubemap);
		data->capture_transforms = app->cubemap_capture_transform_buffer;
		data->skybox_mesh        = &app->skybox_mesh;
		
		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("Irradiance"), R_PassType_Graphics);
		R_PassSetRecord           (pass, R_IBLPassIrradianceFn, data);
		R_PassSetMultiViewMask    (pass, 0b111111);
		R_PassWriteColour         (pass, R_GraphImportTexture(&app->graph, app->irradiance_cubemap), NULL);
	}
	
	// Prefilter.
	{
		// TODO: add prefilter passes.
	}
}

internal void
AppDestroyRender(App *app)
{
	R_DebugRendererDestroy    (&app->debug_renderer);
	R_DeferredRendererDestroy (&app->deferred_renderer);
	R_ShadowRendererDestroy   (&app->shadow_renderer);
	R_CullingDestroy          (&app->culling);

	GFX_DeviceTextureDestroy(&app->graphics_device, app->environment_cubemap);
	GFX_DeviceTextureDestroy(&app->graphics_device, app->irradiance_cubemap);
	GFX_DeviceTextureDestroy(&app->graphics_device, app->prefilter_cubemap);
	
	R_MeshDestroy(&app->skybox_mesh, &app->graphics_device);
	
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
	app->entity_log_channel = osapi->LogOpenChannel(String8Lit("ENTITY"));
	
	ENT_WorldInit(&app->world, &app->entity_arena, app->entity_log_channel);
	ENT_EventQueueInit(&app->events, app->entity_log_channel);
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
   EDITOR
   ================================================== */

internal void
AppInitEditor(App *app)
{
	app->editor_log_channel = osapi->LogOpenChannel(String8Lit("EDITOR"));
	
	EditorInit(&app->editor, &app->editor_arena, app->editor_log_channel);
}

internal void
AppDestroyEditor(App *app)
{
	EditorDestroy(&app->editor);
}

internal void
AppHotLoadEditor(App *app)
{
	EditorHotLoad(&app->editor);
}

internal void
AppHotUnloadEditor(App *app)
{
	EditorHotUnload(&app->editor);
}


/* ==================================================
   APP
   ================================================== */

internal void
AppCoreFatalHandler(const char *file, i32 line, const char *fn, const char *msg)
{
	osapi->Log(LOG_Level_Break, LOG_ChannelNull(), file, line, fn, "%s", msg);
}

internal void
AppInit_(App *app)
{
	CoreSetFatalHandler(AppCoreFatalHandler);
	
	app->log_channel = osapi->LogOpenChannel(String8Lit("APP"));
	
	AppInitGraphics (app);
	AppInitAudio    (app);
	AppInitAssets   (app);
	AppInitRender   (app);
	AppInitEntity   (app);
	AppInitEditor   (app);

	CH_TimerStart(&app->elapsed_timer);
	CH_TimerStart(&app->delta_timer);
	CH_TimerStart(&app->hot_reload_timer);
}

__declspec(dllexport) App *
AppInit(const OS_API *api)
{
	osapi = api;

	Arena bootstrap = ArenaInitReserved(Megabytes(64));
	App *app = ArenaPushArray(&bootstrap, App, 1);
	app->bootstrap_arena = bootstrap;

	app->graphics_arena = ArenaInitReserved(Gigabytes(3));
	app->audio_arena    = ArenaInitReserved(Gigabytes(1));
	app->asset_arena    = ArenaInitReserved(Gigabytes(3));
	app->render_arena   = ArenaInitReserved(Gigabytes(2));
	app->entity_arena   = ArenaInitReserved(Gigabytes(1));
	app->editor_arena   = ArenaInitReserved(Gigabytes(1));
	
	AppInit_(app);

	DebugLogI(app->log_channel, "Initialized.");
	
	return app;
}

__declspec(dllexport) void
AppDestroy(App *app)
{
	GFX_DeviceWaitIdle(&app->graphics_device);

	DebugLogI(app->log_channel, "Destroying...");

	AppDestroyEditor   (app);
	AppDestroyEntity   (app);
	AppDestroyRender   (app);
	AppDestroyAssets   (app);
	AppDestroyAudio    (app);
	AppDestroyGraphics (app);
}

global f32 app_pp_exposure = 1.f;

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

	//DebugPrintT("Hello!!!");

	AST_FlushUploads(&app->assets);

	EditorTick(&app->editor, input, dt, elapsed);
	
	ENT_WorldTickPreAnim(&app->world, &app->events, dt, input);

	// TODO: animation system

	ENT_WorldTickPostAnim(&app->world, &app->events, dt, input);

	if (I_KbDown(input, I_KeyboardKey_Up  ))  app_pp_exposure += dt;
	if (I_KbDown(input, I_KeyboardKey_Down))  app_pp_exposure -= dt;
	
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
	listener.position = app->editor.camera.position;
	listener.direction = app->editor.camera.forward;
	
	//AUD_Tick(&app->audio_system, dt, listener);
	
	GFX_CmdBuffer cmd = GFX_DeviceBeginFrame(&app->graphics_device, &app->swapchain);
	{
		R_SceneDebug(&app->scene);
	
		R_SceneResources scene_resources = R_SceneRefreshTransientResources(&app->scene, &app->frame_upload_ring_buffer);
	
		AppRender(app, dt, elapsed, &cmd, &scene_resources);

		R_GraphCompile(&app->graph, &app->swapchain);
		R_GraphExecute(&app->graph, &app->swapchain, &cmd, &app->scene, &app->editor.camera, dt, elapsed);
		R_GraphPresentToSwapchain(&app->graph, &app->swapchain, &cmd);
		R_GraphReset(&app->graph);
	}
	GFX_DeviceEndFrame(&app->graphics_device, &app->swapchain, &cmd);
	
	ArenaReset(&app->pass_frame_arena);
	
	return false;
}

__declspec(dllexport) void
AppHotLoad(App *app, const OS_API *api)
{
	osapi = api;
	
	CoreSetFatalHandler(AppCoreFatalHandler);
	
	AppHotLoadGraphics   (app);
	AppHotLoadAudio      (app);
	AppHotLoadAssets     (app);
	AppHotLoadRender     (app);
	AppHotLoadEntity     (app);
	AppHotLoadEditor     (app);
}

__declspec(dllexport) void
AppHotUnload(App *app)
{
	AppHotUnloadEditor     (app);
	AppHotUnloadEntity     (app);
	AppHotUnloadRender     (app);
	AppHotUnloadAssets     (app);
	AppHotUnloadAudio      (app);
	AppHotUnloadGraphics   (app);
}

internal void
AppRender(App *app, f32 dt, f32 elapsed, GFX_CmdBuffer *cmd, const R_SceneResources *scene_resources)
{
	R_CameraRecompute(&app->editor.camera);

	u32 window_width, window_height;
	osapi->GetWindowSize(&window_width, &window_height);
	
	R_GPU_FrameData frame_data = {0};
	frame_data.view = app->editor.camera.view;
	frame_data.proj = app->editor.camera.proj;
	frame_data.view_proj = M4MulM4(app->editor.camera.proj, app->editor.camera.view);
	frame_data.view_proj_no_translation = M4MulM4(app->editor.camera.proj, M4RemoveTranslation(app->editor.camera.view));
	frame_data.inv_view = M4Inverse(app->editor.camera.view);
	frame_data.inv_proj = M4Inverse(app->editor.camera.proj);
	frame_data.camera_position = app->editor.camera.position;
	frame_data.window_resolution = v2(window_width, window_height);
	frame_data.time = elapsed;

	GFX_DeviceBufferWrite(&app->graphics_device,
						  app->frame_data_buffer,
						  &frame_data, sizeof(frame_data), 0);

	R_Blackboard bb = {0};

	R_FrustumVolume frustum = R_CameraFrustum(&app->editor.camera);

	R_DrawStream draw_stream = R_CullFrustum(&app->culling,
											 &app->graph,
											 &app->pass_frame_arena,
											 &app->scene,
											 scene_resources,
											 &frustum);

	R_ShadowRendererRender(&app->shadow_renderer,
						   &app->graph,
						   &bb,
						   &app->pass_frame_arena,
						   &app->culling,
						   &app->scene,
						   scene_resources);

	R_DeferredRenderGeometry(&app->deferred_renderer,
							&app->graph,
							&bb,
							&app->pass_frame_arena,
							scene_resources,
							app->frame_data_buffer,
							app->linear_sampler,
							&draw_stream);

	R_GraphTexHandle lighting = R_DeferredRenderLighting(&app->deferred_renderer,
														&app->graph,
														&bb,
														&app->pass_frame_arena,
														scene_resources,
														app->frame_data_buffer,
														app->linear_sampler,
														app->irradiance_cubemap,
														app->prefilter_cubemap,
														app->environment_cubemap); // TODO: proper BRDF LUT texture.

	// -- Skybox
	{
		AST_Handle shader_handle = AST_RequireNow(&app->assets, String8Lit("assets://shaders/passes/skybox.slang"), AST_Type_Shader);
		GFX_ShaderKey shader = AST_Get(&app->assets, shader_handle, AST_Type_Shader)->shader.key;

		R_SkyboxPassData *data = ArenaPushArray(&app->pass_frame_arena, R_SkyboxPassData, 1);
		data->shader = shader;
		data->cubemap = GFX_DeviceTextureViewAuto(&app->graphics_device, app->environment_cubemap);
		data->sampler = app->linear_sampler;
		data->frame_data_buffer = app->frame_data_buffer;
		data->skybox_mesh = &app->skybox_mesh;
		
		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("Skybox"), R_PassType_Graphics);
		R_PassSetRecord(pass, R_SkyboxPassFn, data);
		R_PassReadTextureGraphics(pass, R_GraphImportTexture(&app->graph, app->environment_cubemap));
		lighting = R_PassWriteColour(pass, lighting, NULL);
		bb.gbuffer.depth = R_PassWriteDepth(pass, bb.gbuffer.depth, NULL);
	}

	// -- Post Processing.
	{
		AST_Handle shader_handle = AST_RequireNow(&app->assets, String8Lit("assets://shaders/passes/hdr_tonemapping.comp.slang"), AST_Type_Shader);
		GFX_ShaderKey shader = AST_Get(&app->assets, shader_handle, AST_Type_Shader)->shader.key;
		
		R_PostProcessingPassData *data = ArenaPushArray(&app->pass_frame_arena, R_PostProcessingPassData, 1);
		data->shader = shader;
		data->exposure = app_pp_exposure;
		data->input = lighting;
		data->output = lighting;

		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("Post Processing"), R_PassType_Compute);
		R_PassSetRecord(pass, R_PostProcessingPassFn, data);
		R_PassReadTextureCompute(pass, lighting);
		lighting = R_PassWriteTextureCompute(pass, lighting);
	}

	R_DebugRendererRender(&app->debug_renderer,
						  dt,
						  &app->graph,
						  &app->pass_frame_arena,
						  lighting,
						  bb.gbuffer.depth);

	R_GraphSetBackbuffer(&app->graph, lighting);
}
