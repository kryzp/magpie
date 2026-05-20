
// Todo: Move these out into their respective
//       backends.

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb/stb_image_write.h"

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <vma/vk_mem_alloc.h>

#include "ext/slang/slang_compiler.h"

#define SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
#include "ext/spirv/spirv_reflect.h"
#include "ext/spirv/spirv_reflect.c"

#define MINIAUDIO_IMPLEMENTATION
#include "ext/ma/miniaudio.h"

#define CGLTF_IMPLEMENTATION
#include "ext/gltf/cgltf.h"

// who decided to name these macros ffs
#undef min
#undef max
#undef near
#undef far

// ---

#include "core/core_inc.h"
#include "os/os_inc.h"
#include "core/core_inc.c"
#include "os/os_inc.c"

#include "io/io_inc.h"
#include "io/io_inc.c"

#include "chrono/chrono_inc.h"
#include "chrono/chrono_inc.c"

#include "graphics/graphics_inc.h"
#include "graphics/graphics_inc.c"

#include "audio/audio_inc.h"
#include "audio/audio_inc.c"

#include "asset/asset_inc.h"
#include "asset/asset_inc.c"

#include "animation/animation_inc.h"
#include "animation/animation_inc.c"

#include "render/render_inc.h"
#include "render/render_inc.c"

#include "physics/physics_inc.h"
#include "physics/physics_inc.c"

#include "entity/entity_inc.h"
#include "entity/entity_inc.c"

#include "gamemode/gamemode_inc.h"
#include "gamemode/gamemode_inc.c"

#include "dev/dev_inc.h"
#include "dev/dev_inc.c"

#include "editor/editor_inc.h"
#include "editor/editor_inc.c"

#include "app.h"


/* ==================================================
   GRAPHICS
   ================================================== */

internal void
AppInitGraphics(App *app)
{
	app->graphics_log_channel = osapi->LogChannelOpen(String8Lit("GRAPHICS"));

	
	GFX_DeviceInit(&app->graphics_device, &app->graphics_arena, app->graphics_log_channel);


	app->swapchain = GFX_DeviceSwapchainCreate(&app->graphics_device);

	
	app->shader_compiler_log_channel = osapi->LogChannelOpen(String8Lit("SLANG"));
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

	app->linear_sampler  = GFX_DeviceSamplerCreateF(&app->graphics_device, VK_FILTER_LINEAR);
	app->nearest_sampler = GFX_DeviceSamplerCreateF(&app->graphics_device, VK_FILTER_NEAREST);
}

internal void
AppDestroyGraphics(App *app)
{
	GFX_DeviceSamplerDestroy(&app->graphics_device, app->linear_sampler);
	GFX_DeviceSamplerDestroy(&app->graphics_device, app->nearest_sampler);
	
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

/*
 * Audio is temporarily removed because hot reloading fucks
 * with the library I'm using (miniaudio).
 *
 * I might have to make some modifications to the miniaudio
 * source code to fix that... at some point...
 */

internal void
AppInitAudio(App *app)
{
	/*
	app->audio_log_channel = osapi->LogChannelOpen(String8Lit("AUDIO"));
	
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
	app->asset_log_channel = osapi->LogChannelOpen(String8Lit("ASSETS"));
	
	AST_Init(&app->assets,
			 &app->asset_arena,
			 app->asset_log_channel,
			 &app->graphics_device,
			 &app->shader_compiler,
			 app->audio_backend);

	// I stole this concept of asset mounting from the "Granite" engine / renderer by Themaister.
	// It's so simple but it makes everything so much cleaner!!!
	//AST_Mount(&app->assets, String8Lit("engine://shaders"), String8Lit("src/render/shaders"));
	AST_Mount(&app->assets, String8Lit("assets://"),        String8Lit("res"));
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
	app->render_log_channel = osapi->LogChannelOpen(String8Lit("RENDER"));
	
	R_GraphInit(&app->graph, &app->render_arena, &app->graphics_device,               osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("GRAPH")));
	R_SceneInit(&app->scene, &app->render_arena, &app->graphics_device, &app->assets, osapi->LogChannelOpenFrom(app->render_log_channel, String8Lit("SCENE")));

	AppInitRenderCreateSkyboxMesh(app);

	const u32 prefilter_mips = 5;

	app->brdf_lut            = GFX_DeviceTextureAlloc2D      (&app->graphics_device, 512, 512, VK_FORMAT_R32G32_SFLOAT,       1);
	app->environment_cubemap = GFX_DeviceTextureAllocCubemap (&app->graphics_device, 512,      VK_FORMAT_R32G32B32A32_SFLOAT, 8);
	app->irradiance_cubemap  = GFX_DeviceTextureAllocCubemap (&app->graphics_device,  32,      VK_FORMAT_R32G32B32A32_SFLOAT, 1);
	app->prefilter_cubemap   = GFX_DeviceTextureAllocCubemap (&app->graphics_device, 128,      VK_FORMAT_R32G32B32A32_SFLOAT, prefilter_mips);

	R_CullingInit                (&app->culling,          &app->assets);
	R_ShadowRendererInit         (&app->shadow_renderer,  &app->graphics_device, &app->assets);
	R_ForwardRendererInit        (&app->forward_renderer, &app->graphics_device, &app->assets);
	R_DebugRendererInitAndSelect (&app->debug_renderer,   &app->render_arena, &app->graphics_device, &app->assets);

	AST_Handle brdf_lut_shader_handle   = AST_Require(&app->assets, String8Lit("assets://shaders/passes/ibl/brdf_lut.slang"),                   AST_Type_Shader);
	AST_Handle hdr_to_env_shader_handle = AST_Require(&app->assets, String8Lit("assets://shaders/passes/ibl/hdr_to_environment_cubemap.slang"), AST_Type_Shader);
	AST_Handle irradiance_shader_handle = AST_Require(&app->assets, String8Lit("assets://shaders/passes/ibl/irradiance_convolution.slang"),     AST_Type_Shader);
	AST_Handle prefilter_shader_handle  = AST_Require(&app->assets, String8Lit("assets://shaders/passes/ibl/prefilter_convolution.slang"),      AST_Type_Shader);

	AST_Handle hdr_texture_handle = AST_Require(&app->assets, String8Lit("assets://environment_map_2.hdr"), AST_Type_Texture);
	
	app->object_model_handle = AST_Require(&app->assets, String8Lit("assets://models/Sponza/glTF/Sponza.gltf"),                     AST_Type_Model);
	//app->object_model_handle = AST_Require(&app->assets, String8Lit("assets://models/DamagedHelmet/glTF/DamagedHelmet.gltf"),       AST_Type_Model);
	//app->object_model_handle = AST_Require(&app->assets, String8Lit("assets://models/CompareSheen/glTF/CompareSheen.gltf"),         AST_Type_Model);
	//app->object_model_handle = AST_Require(&app->assets, String8Lit("assets://models/CompareClearcoat/glTF/CompareClearcoat.gltf"), AST_Type_Model);
	//app->object_model_handle = AST_Require(&app->assets, String8Lit("assets://models/SimpleSkin/glTF/SimpleSkin.gltf"),             AST_Type_Model);
	//app->object_model_handle = AST_Require(&app->assets, String8Lit("assets://models/RiggedFigure/glTF/RiggedFigure.gltf"),         AST_Type_Model);
	//app->object_model_handle = AST_Require(&app->assets, String8Lit("assets://models/RiggedSimple/glTF/RiggedSimple.gltf"),         AST_Type_Model);
	
	ScratchArena scratch = ScratchBegin(NULL, 0);
	{
		GFX_CmdBuffer cmd = GFX_DeviceSubmitImBegin(&app->graphics_device);
		R_ModelImportReceipt receipt = R_SceneImportModel(&app->scene, &cmd, scratch.arena, app->object_model_handle, (u32)(-1));
		GFX_DeviceSubmitImEnd(&app->graphics_device, &cmd);
					
		for (u32 i = 0; i < receipt.count; i++)
		{
			R_ModelEntry *entry = &receipt.entries[i];

			R_ObjectDesc desc = {0};
			desc.transform = entry->transform;
			desc.sphere_bounds = entry->sphere_bounds;
			desc.mesh = entry->mesh;
			desc.material = entry->material;

			app->object_handle = R_SceneObjectCreate(&app->scene, &desc);
		}

		//ANIM_AnimatorSelect(&app->object_animator, &app->render_arena, &app->assets, app->object_model_handle);
		//ANIM_AnimatorPlay(&app->object_animator, 0);
	}
	ScratchRelease(&scratch);

	R_Light light = {0};
	light.type = R_LightType_Point;
	light.position = v3(0.f, 0.f, 1.f);
	light.direction = v3x(0.f);
	light.colour = v3(1.f, 1.f, 1.f);
	light.intensity = 5.f;
	light.falloff = 1.f;
	light.casts_shadows = true;
	light.shadow_near = 0.1f;
	light.shadow_far = 10.f;
	
	app->light_handle = R_SceneLightCreate(&app->scene, &light);
	
	R_IrradianceVolumeInit(&app->irradiance_volume,
						   &app->graphics_device, &app->assets,
						   osapi->LogChannelOpen(String8Lit("IRRADIANCE")),
						   v3(-8.f, -6.f,  -1.f),
						   v3( 8.f,  6.f,  12.f),
						   1, 1, 1,
						   &app->skybox_mesh,
						   GFX_DeviceTextureViewAuto(&app->graphics_device, app->environment_cubemap),
						   app->linear_sampler);
	
	GFX_ShaderKey brdf_lut_shader        = AST_AssetShaderGet(AST_GetNow(&app->assets, brdf_lut_shader_handle,   AST_Type_Shader));
	GFX_ShaderKey hdr_to_env_shader      = AST_AssetShaderGet(AST_GetNow(&app->assets, hdr_to_env_shader_handle, AST_Type_Shader));
	GFX_ShaderKey irradiance_pass_shader = AST_AssetShaderGet(AST_GetNow(&app->assets, irradiance_shader_handle, AST_Type_Shader));
	GFX_ShaderKey prefilter_pass_shader  = AST_AssetShaderGet(AST_GetNow(&app->assets, prefilter_shader_handle,  AST_Type_Shader));
	
	GFX_TextureKey hdr_texture_gfx = AST_GetNow(&app->assets, hdr_texture_handle, AST_Type_Texture)->texture.key;

	// Generate BRDF Lookup Table.
	{
		R_BRDFLutPassData *data = ArenaPushArray(&app->frame_arena, R_BRDFLutPassData, 1);
		data->shader = brdf_lut_shader;
		
		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("BRDF LUT"), R_PassType_Graphics);
		R_PassSetRecord   (pass, R_BRDFLutPassFn, data);
		R_PassWriteColour (pass, R_GraphImportTexture(&app->graph, app->brdf_lut), NULL);
	}
	
	// Generate Environment Cubemap.
	{
		R_HdrToEnvPassData *data = ArenaPushArray(&app->frame_arena, R_HdrToEnvPassData, 1);
		data->shader             = hdr_to_env_shader;
		data->sampler            = app->linear_sampler;
		data->hdr_view           = GFX_DeviceTextureViewAuto(&app->graphics_device, hdr_texture_gfx);
		data->capture_transforms = app->cubemap_capture_transform_buffer;
		data->skybox_mesh        = &app->skybox_mesh;
		
		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("HDR -> Environment Map"), R_PassType_Graphics);
		R_PassSetRecord        (pass, R_HdrToEnvPassFn, data);
		R_PassSetMultiViewMask (pass, 0b111111);
		R_PassWriteColour      (pass, R_GraphImportTexture(&app->graph, app->environment_cubemap), NULL);

		R_GenerateMipsPassData *mips_data = ArenaPushArray(&app->frame_arena, R_GenerateMipsPassData, 1);
		mips_data->texture = app->environment_cubemap;
		
		R_Pass *pass_mipmaps = R_GraphAdd(&app->graph, String8Lit("Environment Map Mipmapping"), R_PassType_Transfer);
		R_PassSetRecord      (pass_mipmaps, R_GenerateMipsPassFn, mips_data);
		R_PassBlitTextureDst (pass_mipmaps, R_GraphImportTexture(&app->graph, app->environment_cubemap));
	}
	
	// Irradiance.
	{
		R_IBLPassIrradianceData *data = ArenaPushArray(&app->frame_arena, R_IBLPassIrradianceData, 1);
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
		const u32 mipmap_count = prefilter_mips;
		
		for (u32 i = 0; i < mipmap_count; i++)
		{
			R_IBLPassPrefilterData *data = ArenaPushArray(&app->frame_arena, R_IBLPassPrefilterData, 1);
			data->shader             = prefilter_pass_shader;
			data->sampler            = app->linear_sampler;
			data->env_view           = GFX_DeviceTextureViewAuto(&app->graphics_device, app->environment_cubemap);
			data->capture_transforms = app->cubemap_capture_transform_buffer;
			data->skybox_mesh        = &app->skybox_mesh;
			data->roughness          = (f32)i / (f32)(mipmap_count - 1);

			GFX_SubresourceRange range = {0};
			range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
			range.base_mip = i;
			range.mips = 1;
			range.base_layer = 0;
			range.layers = 6;
		
			R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("Prefilter"), R_PassType_Graphics);
			R_PassSetRecord           (pass, R_IBLPassPrefilterFn, data);
			R_PassSetMultiViewMask    (pass, 0b111111);
			R_PassWriteColourEx       (pass, R_GraphImportTexture(&app->graph, app->prefilter_cubemap), NULL, range);
		}
	}
}

internal void
AppDestroyRender(App *app)
{
	R_IrradianceVolumeDestroy (&app->irradiance_volume);
	R_DebugRendererDestroy    (&app->debug_renderer);
	R_ForwardRendererDestroy  (&app->forward_renderer);
	R_ShadowRendererDestroy   (&app->shadow_renderer);
	R_CullingDestroy          (&app->culling);

	GFX_DeviceTextureDestroy(&app->graphics_device, app->brdf_lut);
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
	R_DebugRendererSelect(&app->debug_renderer);
}

internal void
AppHotUnloadRender(App *app)
{
}

/* ==================================================
   ENTITY
   ================================================== */

internal void
AppInitPhysics(App *app)
{
	app->physics_log_channel = osapi->LogChannelOpen(String8Lit("PHYSICS"));

	PHYS_EngineInit(&app->physics_engine, app->physics_log_channel);
}

internal void
AppDestroyPhysics(App *app)
{
	PHYS_EngineDestroy(&app->physics_engine);
}

internal void
AppHotLoadPhysics(App *app)
{
}

internal void
AppHotUnloadPhysics(App *app)
{
}


/* ==================================================
   ENTITY
   ================================================== */

internal void
AppInitEntity(App *app)
{
	app->entity_log_channel = osapi->LogChannelOpen(String8Lit("ENTITY"));
	
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
	app->editor_log_channel = osapi->LogChannelOpen(String8Lit("EDITOR"));

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
 * But I have one final question,
 * what is it, you fear most?
 */

internal void
AppInit_(App *app)
{
	app->log_channel = osapi->LogChannelOpen(String8Lit("APP"));
	
	AppInitGraphics (app);
	AppInitAudio    (app);
	AppInitAssets   (app);
	AppInitRender   (app);
	AppInitPhysics  (app);
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

	Arena bootstrap = ArenaAlloc(sizeof(App));
	App *app = ArenaPushArray(&bootstrap, App, 1);
	app->bootstrap_arena = bootstrap;

	app->graphics_arena = ArenaAlloc(Gigabytes(3));
	app->audio_arena    = ArenaAlloc(Gigabytes(1));
	app->asset_arena    = ArenaAlloc(Gigabytes(3));
	app->render_arena   = ArenaAlloc(Gigabytes(2));
	app->physics_arena  = ArenaAlloc(Gigabytes(1));
	app->entity_arena   = ArenaAlloc(Gigabytes(1));
	app->editor_arena   = ArenaAlloc(Gigabytes(1));
	
	app->frame_arena    = ArenaAlloc(Gigabytes(1));
	
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
	AppDestroyPhysics  (app);
	AppDestroyRender   (app);
	AppDestroyAssets   (app);
	AppDestroyAudio    (app);
	AppDestroyGraphics (app);

	ArenaRelease(&app->frame_arena);
	
	ArenaRelease(&app->editor_arena);
	ArenaRelease(&app->entity_arena);
	ArenaRelease(&app->physics_arena);
	ArenaRelease(&app->render_arena);
	ArenaRelease(&app->asset_arena);
	ArenaRelease(&app->audio_arena);
	ArenaRelease(&app->graphics_arena);
	
	DebugLogI(app->log_channel, "Destroyed");

	// TODO: fuck are we remembering to release this? why wasnt it crashing lol
	//ArenaRelease(&app->bootstrap_arena);
}

global f32 app_pp_exposure = 1.f;

internal void
AppLogFPS(App *app, f32 dt)
{
	static u32 index = 0;
	static f32 fps_history[512] = {0};

	const f32 fps_now = 1.f / dt;

	fps_history[index % ArraySize(fps_history)] = fps_now;
	index++;

	f32 fps_avg = 0.f;

	for (u32 i = 0; i < ArraySize(fps_history); i++)
		fps_avg += fps_history[i];
	
	fps_avg /= (f32)ArraySize(fps_history);
	
	//DebugLogT(app->log_channel, "FPS: %.2f", fps_avg);
}

__declspec(dllexport) b32
AppTick(App *app, const OS_InputState *input)
{
	if (OS_KbPressed(input, OS_KeyboardKey_Escape))
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

	if (OS_KbPressed(input, OS_KeyboardKey_Enter))
	{
		R_IrradianceVolumeBake(&app->irradiance_volume, &app->scene);
	}

	AST_FlushUploads(&app->assets);

	EditorTick(&app->editor, input, dt, elapsed);
	
	ENT_WorldTickPreAnim(&app->world, &app->events, dt, input);

	// TODO: animation system

	ENT_WorldTickPostAnim(&app->world, &app->events, dt, input);

	if (OS_KbDown(input, OS_KeyboardKey_Up  ))  app_pp_exposure += dt;
	if (OS_KbDown(input, OS_KeyboardKey_Down))  app_pp_exposure -= dt;
	
	f32 clamped_delta = dt;

	if (dt > max_frame_time)
	{
		DebugLogW(app->log_channel, "Had to clamp delta (was %f, clamped to %f).", dt, max_frame_time);
		clamped_delta = max_frame_time;
	}

	AppLogFPS(app, dt);
	
	app->delta_accumulator += clamped_delta;

	while (app->delta_accumulator >= fixed_dt)
	{
		PHYS_EngineTick(&app->physics_engine, fixed_dt);
		app->delta_accumulator -= fixed_dt;
	}

	R_SceneLightSetPosition(&app->scene, app->light_handle, v3(SinF(elapsed*2.f)*2.f, 0.f, 1.f));
	
	ENT_WorldTickPostPhysics(&app->world, &app->events, dt, input);

	ENT_EventDispatch(&app->events, &app->world);

	ENT_WorldFlush(&app->world);
	
	AUD_Listener listener = {0};
	listener.position = app->editor.camera.position;
	listener.direction = app->editor.camera.forward;

	//AUD_Tick(&app->audio_system, dt, listener);

	//R_IrradianceVolumeDebug(&app->irradiance_volume);
	//R_SceneDebug(&app->scene);

	/*
	ANIM_AnimatorTick(&app->object_animator, &app->assets, dt);
	ANIM_Palette palette = ANIM_AnimatorPalette(&app->object_animator, 0);
	R_SceneObjectSetSkinning(&app->scene, app->object_handle, palette.palette, palette.joint_count);
	*/
	
	GFX_CmdBuffer cmd = GFX_DeviceBeginFrame(&app->graphics_device, &app->swapchain);
	{
		AppRender(app, dt, elapsed, &cmd);

		/*
		R_TextureInfo swapchain_attachment_info = R_TextureInfoInit();
		swapchain_attachment_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		R_GraphTexHandle dummy_handle = R_GraphCreateTexture(&app->graph, &swapchain_attachment_info);
		R_Clear clear = R_ClearColour((f32)I_KbDown(input, I_KeyboardKey_Tab), 0.0f, 0.0f, 1.f);
		R_Pass *dummy = R_GraphAdd(&app->graph, String8Lit("dummy"), R_PassType_Graphics);
		R_PassWriteColour(dummy, dummy_handle, &clear);
		R_GraphSetBackbuffer(&app->graph, dummy_handle);
		*/
		
		R_GraphCompile(&app->graph, &app->swapchain);
		R_GraphExecute(&app->graph, &app->swapchain, &cmd, &app->scene, &app->editor.camera, dt, elapsed);
		R_GraphPresentToSwapchain(&app->graph, &app->swapchain, &cmd);
		R_GraphReset(&app->graph);
	}
	GFX_DeviceEndFrame(&app->graphics_device, &app->swapchain, &cmd);

	GFX_RingBufferReset(&app->frame_upload_ring_buffer);
	ArenaReset(&app->frame_arena);
	
	return false;
}

__declspec(dllexport) void
AppHotLoad(App *app, const OS_API *api)
{
	osapi = api;
	
	AppHotLoadGraphics     (app);
	AppHotLoadAudio        (app);
	AppHotLoadAssets       (app);
	AppHotLoadRender       (app);
	AppHotLoadEntity       (app);
	AppHotLoadEditor       (app);
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
AppRender(App *app, f32 dt, f32 elapsed, GFX_CmdBuffer *cmd)
{
	R_SceneFrameData scene_resources = R_SceneUploadFrameData(&app->scene, &app->frame_upload_ring_buffer);

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

	R_Bulletin bt = {0};
	bt.pass_arena = &app->frame_arena;
	bt.frame_data_buffer = app->frame_data_buffer;
	bt.brdf = app->brdf_lut;
	bt.linear_sampler = app->linear_sampler;
	bt.nearest_sampler = app->nearest_sampler;
	bt.scene_resources = &scene_resources;
	bt.irradiance_volume = &app->irradiance_volume;
	bt.irradiance_fallback_cubemap = app->irradiance_cubemap;
	bt.prefilter_cubemap = app->prefilter_cubemap;
	bt.brdf = app->brdf_lut;
	
	R_Blackboard bb = {0};

	R_FrustumVolume frustum = R_CameraFrustum(&app->editor.camera);

	R_ShadowRendererUploadGPU(&app->shadow_renderer, &app->scene);

	R_ShadowRendererRender(&app->shadow_renderer,
						   &app->graph,
						   &bt, &bb,
						   &app->scene,
						   &app->culling);

	R_DrawStream draw_stream = R_CullFrustum(&app->culling,
											 &app->graph,
											 &bt,
											 R_CullFilter_OpaqueOnly,
											 &frustum);

	R_ForwardRender(&app->forward_renderer,
					&app->graph,
					&bt, &bb,
					&draw_stream);
		
	// Skybox.
	{
		AST_Handle shader_handle = AST_Require(&app->assets, String8Lit("assets://shaders/passes/post/skybox.slang"), AST_Type_Shader);
		GFX_ShaderKey shader = AST_AssetShaderGet(AST_GetNow(&app->assets, shader_handle, AST_Type_Shader));

		
		R_SkyboxPassData *data = ArenaPushArray(&app->frame_arena, R_SkyboxPassData, 1);
		data->shader = shader;
		data->cubemap = GFX_DeviceTextureViewAuto(&app->graphics_device, app->environment_cubemap);
		data->sampler = app->linear_sampler;
		data->frame_data_buffer = app->frame_data_buffer;
		data->skybox_mesh = &app->skybox_mesh;

		
		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("Skybox"), R_PassType_Graphics);
		bb.lighting.resolved = R_PassWriteColourResolve(pass, bb.lighting.msaa, bb.lighting.resolved, NULL);
		bb.depth.resolved    = R_PassWriteDepthResolve(pass, bb.depth.msaa,    bb.depth.resolved,    NULL);
		R_PassReadTextureGraphics(pass, R_GraphImportTexture(&app->graph, app->environment_cubemap));
		R_PassSetRecord(pass, R_SkyboxPassFn, data);
	}
	
	// Post Processing.
	{
		AST_Handle shader_handle = AST_Require(&app->assets, String8Lit("assets://shaders/passes/post/hdr_tonemapping.slang"), AST_Type_Shader);
		GFX_ShaderKey shader = AST_AssetShaderGet(AST_GetNow(&app->assets, shader_handle, AST_Type_Shader));

		
		R_PostProcessingPassData *data = ArenaPushArray(&app->frame_arena, R_PostProcessingPassData, 1);
		data->shader = shader;
		data->exposure = app_pp_exposure;
		data->input = bb.lighting.resolved;
		data->output = bb.lighting.resolved;

		
		R_Pass *pass = R_GraphAdd(&app->graph, String8Lit("Post Processing"), R_PassType_Compute);
		bb.lighting.resolved = R_PassWriteTextureCompute(pass, bb.lighting.resolved);
		R_PassReadTextureCompute(pass, bb.lighting.resolved);
		R_PassSetRecord(pass, R_PostProcessingPassFn, data);
	}

	R_DebugRendererRender(&app->debug_renderer,
						  dt,
						  &app->graph,
						  &app->frame_arena,
						  bb.lighting.resolved,
						  bb.depth.resolved);
	
	R_GraphSetBackbuffer(&app->graph, bb.lighting.resolved);
	R_GraphSetPresentFilter(&app->graph, VK_FILTER_NEAREST);
}
