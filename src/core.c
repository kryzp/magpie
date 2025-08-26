#define VOLK_IMPLEMENTATION
#include "ext/volk.h"
#include "ext/vk_mem_alloc.h"
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
#include "mesh_pass.h"
#include "render_graph.h"
#include "render_context.h"
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
#include "render_graph.c"
#include "render_context.c"
#include "scene.c"
#include "mesh_pass.c"
#include "renderer.c"

internal void
CoreResetGlobals(Platform *platform_)
{
	platform = platform_;
	core = platform->permanent_memory;
	graphics_device = &core->graphics_device;
	vertex_formats = &core->vertex_formats;
}

__declspec(dllexport) void
CoreInit(Platform *platform_)
{
	CoreResetGlobals(platform_);
	
	MemoryArena permanent_arena = MemoryArenaInit(platform->permanent_memory, platform->permanent_memory_size);
	
	core = MemoryArenaPush(&permanent_arena, sizeof(Core));
	
	core->permanent_arena = permanent_arena;
	core->frame_arena = MemoryArenaInit(platform->transient_memory, platform->transient_memory_size);
	
	core->scratch_arenas[0] = MemoryArenaInit(platform->scratch_memory[0], platform->scratch_memory_size);
	core->scratch_arenas[1] = MemoryArenaInit(platform->scratch_memory[1], platform->scratch_memory_size);
	
	AssetsInit(&core->assets, &core->permanent_arena);
	GraphicsDeviceInit(&core->permanent_arena);
	VertexFormatsInit(&core->vertex_formats);
	RenderContextInit(&core->render_context, &core->permanent_arena);
	RendererInit(&core->renderer);
	
	u32 environment_asset_handle = AssetsLoadTexture(&core->assets, str8("res/environment_map.hdr"));
	RenderContextSetSkybox(&core->render_context, &core->render_graph, AssetsImageFromHandle(&core->assets, environment_asset_handle));
	
	RenderContextGenerateBRDFLookUp(&core->render_context, &core->render_graph);
	
	//BRDFRendererGenerateBRDFLookUp(&core->render_context.brdf_lut_image, &core->render_graph);
	//IBLRendererGenerateEnvironmentProbe(&core->render_context.environment_probe, &core->render_graph);
	
	SceneInit(&core->scene);
	
	core->main_camera = CameraInitPerspective(v3(0.f, 0.f, 0.f),
											  v3(0.f, 1.f, 0.f),
											  100.f, 1280.f/720.f,
											  .1f, 10.f);
	
	u32 damaged_helmet_model_asset_handle = AssetsLoadModel(&core->assets, str8("res/DamagedHelmet/DamagedHelmet.gltf"));
	Model *damaged_helmet_model = AssetsModelFromHandle(&core->assets, damaged_helmet_model_asset_handle);
	
	core->damaged_helmet_object = SceneRegisterObject(&core->scene, &core->render_context, &core->assets,
													  &damaged_helmet_model->sub_models[0].mesh,
													  &damaged_helmet_model->sub_models[0].material,
													  m4(1.f),
													  SceneObjectFlag_DrawForwardPass);
	
	core->starting_ticks = platform->GetPerformanceCounter();
}

__declspec(dllexport) void
CoreUpdate(Platform *platform_)
{
	f32 t = GetTotalElapsedSecondsF();
	
	MemoryArenaClear(&core->frame_arena);
	MemoryArenaClear(&core->scratch_arenas[0]);
	MemoryArenaClear(&core->scratch_arenas[1]);
	
	if(platform->kb_pressed[KeyboardKey_Escape])
	{
		DebugLog("Quitting...");
		platform->exit = 1;
	}
	
	// NOTE(kp): Camera.
	{
		core->main_camera.position = v3(0.f, -2.f, 0.f);
		core->main_camera.forward = v3(0.f, 1.f, 0.f);
		
		CameraRecompute(&core->main_camera);
	}
	
	// NOTE(kp): Scene.
	SceneResolveAdding(&core->scene);
	{
		SceneObjectFromHandle(&core->scene, core->damaged_helmet_object)
			->transform = M4Transform(v3(0.f, 5.f, .4f*SinF(t)),
									  QuatInitEuler(0.f, t, 0.f),
									  v3(1.f, 1.f, 1.f),
									  v3(0.f, 0.f, 0.f));
	}
	SceneResolveRemoving(&core->scene);
	
	// NOTE(kp): Rendering.
	core->render_context.cmd = BeginGraphicsPresent();
	{
		Camera *camera = &core->main_camera;
		RenderContextPerFrameData *current_frame = RenderContextGetCurrentFrame(&core->render_context);
		
		core->render_context.mesh_pass = MeshPassInit(&core->scene,
													  &core->frame_arena);
		
		for(i32 i = 0; i < core->scene.object_count; i++)
		{
			SceneObject *object = core->scene.objects + i;
			
			GPU_ObjectData object_data = {0};
			object_data.model_matrix = object->transform;
			object_data.normal_matrix = M4Inverse(M4Transpose(object->transform));
			
			GPUBufferWrite(&current_frame->object_buffer, &object_data,
						   sizeof(GPU_ObjectData),
						   sizeof(GPU_ObjectData) * i);
			
			VkDrawIndexedIndirectCommand command = {0};
			command.indexCount = core->render_context.meshes[object->mesh_id].index_count;
			command.instanceCount = 1;
			command.firstIndex = 0;
			command.vertexOffset = 0;
			command.firstInstance = i;
			
			GPUBufferWrite(&current_frame->indirect_buffer, &command,
						   sizeof(VkDrawIndexedIndirectCommand),
						   sizeof(VkDrawIndexedIndirectCommand) * i);
		}
		
		// NOTE(kp): Per-frame buffer
		{
			GPU_FrameData frame_data = {0};
			frame_data.view = camera->view;
			frame_data.projection = camera->projection;
			frame_data.view_projection = M4MultiplyM4(frame_data.projection, frame_data.view);
			frame_data.view_projection_no_translation = M4MultiplyM4(frame_data.projection, M4RemoveTranslation(frame_data.view));
			frame_data.inv_view = M4Inverse(frame_data.view);
			frame_data.inv_projection = M4Inverse(frame_data.projection);
			frame_data.camera_position.xyz = camera->position;
			frame_data.window_resolution.x = platform->window_pixel_width;
			frame_data.window_resolution.y = platform->window_pixel_height;
			frame_data.time = GetTotalElapsedSecondsF();
			
			GPUBufferWrite(&current_frame->frame_data_buffer, &frame_data,
						   sizeof(GPU_FrameData), 0);
		}
		
		
		RendererRenderFrame(&core->renderer,
							&core->render_context,
							&core->render_graph,
							&core->scene,
							&core->main_camera);
	}
	RenderGraphExecuteRenderPasses(&core->render_graph, &core->render_context);
	EndGraphicsPresent(&core->render_context.cmd);
}

__declspec(dllexport) void
CoreDestroy(Platform *platform_)
{
	SceneDestroy(&core->scene);
	AssetsDestroy(&core->assets);
	RendererDestroy(&core->renderer);
	RenderContextDestroy(&core->render_context);
	GraphicsDeviceDestroy();
}

__declspec(dllexport) void
CoreBeforeHotReload(Platform *platform_)
{
	GraphicsDeviceBeforeHotReload();
}

__declspec(dllexport) void
CoreAfterHotReload(Platform *platform_)
{
	CoreResetGlobals(platform_);
	GraphicsDeviceAfterHotReload();
}
