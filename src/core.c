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
#include "render_state.h"
#include "renderer.h"
#include "vertex_formats.h"
#include "shaders.h"
#include "core.h"
#include "scratch.h"

#include "timer.c"
#include "scratch.c"
#include "hash_table.c"
#include "graphics_device.c"
#include "vertex_formats.c"
#include "shaders.c"
#include "model.c"
#include "assets.c"
#include "render_graph.c"
#include "render_state.c"
#include "scene.c"
#include "mesh_pass.c"
#include "renderer.c"
#include "skybox.c"
#include "ibl_renderer.c"
#include "brdf_lut.c"

// https://songho.ca/opengl/gl_sphere.html
// TODO: Use a more efficient sphere shape like an ICOSPHERE or CUBESPHERE.
internal void CoreCreateUnitSphereMesh()
{
	ScratchArena scratch = GetScratch(&core->permanent_arena, 1);

	u16 sector_count = 10;
	u16 stack_count = 10;

	f32 sector_step = 2.f * PIf / (f32)sector_count;
	f32 stack_step = PIf / (f32)stack_count;

	u32 vertex_count = (stack_count + 1) * (sector_count + 1);
	u32 index_count = sector_count * (stack_count - 1) * 6;

	v3 *vertices = MemoryArenaPush(scratch.arena, sizeof(v3) * vertex_count);
	u16 *indices = MemoryArenaPush(scratch.arena, sizeof(u16) * index_count);

	u32 index = 0;

	for (i32 i = 0; i <= stack_count; i++) {
		f32 theta = PIf / 2.f - i * stack_step;
		for (i32 j = 0; j <= sector_count; j++) {
			f32 phi = j * sector_step;
			vertices[index++] = SphericalToCartesian(1.f, phi, theta);
		}
	}

	index = 0;

	for (u16 i = 0; i < stack_count; i++) {
		u16 k1 = i * (sector_count + 1); // Current stack.
		u16 k2 = k1 + (sector_count + 1); // Next stack.

		for (u16 j = 0; j < sector_count; j++, k1++, k2++) {
			if (i != 0) {
				indices[index + 0] = k1;
				indices[index + 1] = k1 + 1u;
				indices[index + 2] = k2;

				index += 3;
			}

			if (i != stack_count - 1) {
				indices[index + 0] = k1 + 1u;
				indices[index + 1] = k2 + 1u;
				indices[index + 2] = k2;

				index += 3;
			}
		}
	}

	core->light_sphere_mesh = MeshInit(&vertex_formats->vec3,
					   vertex_count, vertices,
					   index_count, indices);

	ReleaseScratch(&scratch);
}

internal void CoreResetGlobals(Platform *platform_)
{
	platform = platform_;
	core = platform->permanent_memory;
	graphics_device = &core->graphics_device;
	vertex_formats = &core->vertex_formats;
	shaders = &core->shaders;
}

__declspec(dllexport) void CoreInit(Platform *platform_)
{
	CoreResetGlobals(platform_);

	MemoryArena permanent_arena = MemoryArenaInit(platform->permanent_memory, platform->permanent_memory_size);

	core = MemoryArenaPush(&permanent_arena, sizeof(Core));

	core->permanent_arena = permanent_arena;
	core->frame_arena = MemoryArenaInit(platform->transient_memory,
					    platform->transient_memory_size);

	core->scratch_arenas[0] = MemoryArenaInit(platform->scratch_memory[0], platform->scratch_memory_size);
	core->scratch_arenas[1] = MemoryArenaInit(platform->scratch_memory[1], platform->scratch_memory_size);

	AssetsInit(&core->assets, &core->permanent_arena);
	GraphicsDeviceInit(&core->permanent_arena);
	VertexFormatsInit(&core->vertex_formats);
	ShadersInit(&core->shaders, &core->permanent_arena);

	CoreCreateUnitSphereMesh();

	core->linear_sampler = SamplerInitFilter(VK_FILTER_NEAREST);

	m4 capture_projection_matrix = M4Perspective(90.f, 1.f, 0.1f, 10.f);

	// Renderman introduced the left-handed Y-up cubemap in 1990
	// but we use right-handed Z-up so we have to flip these weirdly.
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f,  0.f,  0.f), v3(0.f,  0.f, 1.f)), // X+
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f,  0.f,  0.f), v3(0.f,  0.f, 1.f)), // X-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,  0.f,  1.f), v3(0.f, -1.f, 0.f)), // Y+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,  0.f, -1.f), v3(0.f,  1.f, 0.f)), // Y-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,  1.f,  0.f), v3(0.f,  0.f, 1.f)), // Z+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, -1.f,  0.f), v3(0.f,  0.f, 1.f)), // Z-
	};

	core->cubemap_capture_transforms = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
							  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							  sizeof(m4) * 6);

	for (i32 i = 0; i < 6; i++) {
		m4 m = M4MultiplyM4(capture_projection_matrix, capture_view_matrices[i]);
		GPUBufferWrite(&core->cubemap_capture_transforms, &m,
			       sizeof(m4),
			       sizeof(m4) * i);
	}

	v3 vertices[] = {
		{ -1.f,  1.f,  1.f },
		{ -1.f,  1.f, -1.f },
		{  1.f,  1.f, -1.f },
		{  1.f,  1.f,  1.f },
		{ -1.f, -1.f,  1.f },
		{ -1.f, -1.f, -1.f },
		{  1.f, -1.f, -1.f },
		{  1.f, -1.f,  1.f }
	};
	
	u16 indices[] = {
		0, 2, 1,
		2, 0, 3,
		
		7, 5, 6,
		5, 7, 4,
		
		4, 1, 5,
		1, 4, 0,
		
		3, 6, 2,
		6, 3, 7,
		
		1, 6, 5,
		6, 1, 2,
		
		4, 3, 0,
		3, 4, 7
	};

	core->skybox_mesh = MeshInit(&vertex_formats->vec3,
				     ArraySize(vertices), vertices,
				     ArraySize(indices), indices);

	RenderStateInit(&core->render_state);
	RendererInit(&core->renderer);

	u32 environment_asset_handle = AssetsLoadTexture(&core->assets, str8("res/environment_map.hdr"));

	core->skybox_cubemap = ImageAllocCubemap(1024, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
	EnvironmentMapFromHDR(&core->render_graph, &core->skybox_cubemap, AssetsImageFromHandle(&core->assets, environment_asset_handle));

	core->brdf_lut_image = ImageAlloc2D(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);
	BRDFLookUpGenerate(&core->render_graph, &core->brdf_lut_image);

	core->environment_probe = EnvironmentProbeInit();
	IBLRendererGenerateEnvironmentProbe(&core->render_graph, &core->environment_probe, FetchStandardImageView(&core->skybox_cubemap));

	SceneInit(&core->scene);

	core->main_camera = CameraInitPerspective(v3(0.f, 0.f, 0.f),
						  v3(0.f, 1.f, 0.f),
						  100.f,
						  1280.f / 720.f,
						  .1f,
						  20.f);

	u32 damaged_helmet_model_asset_handle = AssetsLoadModel(&core->assets, str8("res/DamagedHelmet/DamagedHelmet.gltf"));
	Model *damaged_helmet_model = AssetsModelFromHandle(&core->assets, damaged_helmet_model_asset_handle);

	for (i32 i = 0; i < ArraySize(core->damaged_helmet_objects); i++) {
		core->damaged_helmet_objects[i] = SceneRegisterObject(&core->scene, m4(1.f));
		SceneObjectAddMesh(&core->scene, core->damaged_helmet_objects[i],
				   &core->render_state, &core->assets,
				   &damaged_helmet_model->sub_models[0].mesh,
				   &damaged_helmet_model->sub_models[0].material,
				   SceneObjectFlag_DrawDeferredPass);
	}

	Light my_light = {0};
	my_light.type = LightType_Point;
	my_light.colour = v3(1.f, 1.f, 1.f);
	my_light.intensity = 10.f;
	my_light.falloff = 1.f;

	core->light = SceneRegisterObject(&core->scene, m4(1.f));
	SceneObjectAddLight(&core->scene, core->light, &core->render_state, &my_light);

	core->starting_ticks = platform->GetPerformanceCounter();
}

__declspec(dllexport) void CoreUpdate(Platform *platform_)
{
	f32 t = GetTotalElapsedSecondsF();

	MemoryArenaClear(&core->frame_arena);
	MemoryArenaClear(&core->scratch_arenas[0]);
	MemoryArenaClear(&core->scratch_arenas[1]);

	if (platform->kb_pressed[KeyboardKey_Escape]) {
		DebugLog("Quitting...");
		platform->exit = true;
	}

	// Camera.
	core->main_camera.position = v3(0.f, -2.f, 0.f);
	core->main_camera.forward = v3(0.f, 1.f, 0.f);

	CameraRecompute(&core->main_camera);

	// Scene.
	for (i32 i = 0; i < ArraySize(core->damaged_helmet_objects); i++) {
		SceneObjectFromHandle(&core->scene, core->damaged_helmet_objects[i])
			->transform = M4Transform(v3(i, 2.f, .4f * SinF(t)),
						  QuatInitEuler(0.f, t, 0.f),
						  v3u(1.f),
						  v3u(0.f));
	}

	SceneObjectFromHandle(&core->scene, core->light)->transform = M4Transform(v3(SinF(t), 2.f, 1.f),
										  QuatInitIdentity(),
										  v3u(1.f),
										  v3u(0.f));
		
	SceneResolveRemoving(&core->scene);

	// Rendering.
	core->render_state.cmd = BeginGraphicsPresent();
	{
		Camera *camera = &core->main_camera;
		RenderStateFrameData *current_frame = RenderStateGetCurrentFrameData(&core->render_state);
		
		core->render_state.mesh_pass = MeshPassInit(&core->scene, &core->frame_arena);

		i32 mesh_count = 0;
		i32 light_count = 0;

		for (i32 i = 0; i < core->scene.object_count; i++) {
			SceneObject *object = &core->scene.objects[i];
			
			// If the object has a mesh.
			if (object->mesh_id != SCENE_INVALID_HANDLE) {
				GPU_ObjectData object_data = {0};
				object_data.model_matrix = object->transform;
				object_data.normal_matrix = M4Inverse(M4Transpose(object->transform));

				GPUBufferWrite(&current_frame->object_buffer,
					       &object_data,
					       sizeof(GPU_ObjectData),
					       sizeof(GPU_ObjectData) * mesh_count);
				
				VkDrawIndexedIndirectCommand command = {0};
				command.indexCount = core->render_state.meshes[object->mesh_id].index_count;
				command.instanceCount = 1;
				command.firstIndex = 0;
				command.vertexOffset = 0;
				command.firstInstance = mesh_count;

				GPUBufferWrite(&current_frame->indirect_buffer,
					       &command,
					       sizeof(VkDrawIndexedIndirectCommand),
					       sizeof(VkDrawIndexedIndirectCommand) * mesh_count);

				mesh_count++;
			}

			// If the object has a light.
			if (object->light_id != SCENE_INVALID_HANDLE) {
				Light *light = &core->render_state.lights[object->light_id];
			
				f32 epsilon_intensity = .1f;
				f32 light_max = V3MaxValue(light->colour);
				f32 heuristic_radius = SquareRoot((light->intensity * light_max) / (light->falloff * epsilon_intensity));

				GPU_Light gpu_light = {0};
				gpu_light.position     = object->transform.c3; // Last column of transformation matrix is translation.
				gpu_light.colour.xyz   = light->colour;
				gpu_light.colour.w     = light->intensity;
				gpu_light.attenuation  = v4(light->falloff, 0.f, 0.f, 0.f);
				gpu_light.transform    = M4Transform(gpu_light.position.xyz,
								     QuatInitIdentity(),
								     v3u(heuristic_radius),
								     v3u(0.f));

				// Currently, since the light buffer is the same size as the object buffer anyway, we can keep it pretty
				// sparse, and so just write to the current object index. Objects with no light (light id = (u32)(-1))
				// will just not get written.
				GPUBufferWrite(&current_frame->light_buffer,
					       &gpu_light,
					       sizeof(GPU_Light),
					       sizeof(GPU_Light) * light_count);

				light_count++;
			}

		}

		// Per-frame buffer
		{
			GPU_FrameData frame_data = {0};
			frame_data.view = camera->view;
			frame_data.projection = camera->projection;
			frame_data.view_projection = M4MultiplyM4(frame_data.projection, frame_data.view);
			frame_data.view_projection_no_translation = M4MultiplyM4(frame_data.projection,
										 M4RemoveTranslation(frame_data.view));
			frame_data.inv_view = M4Inverse(frame_data.view);
			frame_data.inv_projection = M4Inverse(frame_data.projection);
			frame_data.camera_position.xyz = camera->position;
			frame_data.window_resolution.x = platform->window_pixel_width;
			frame_data.window_resolution.y = platform->window_pixel_height;
			frame_data.time = GetTotalElapsedSecondsF();

			GPUBufferWrite(&current_frame->frame_data_buffer,
				       &frame_data,
				       sizeof(GPU_FrameData), 0);
		}

		RendererRenderFrame(&core->renderer,
				    &core->render_state,
				    &core->render_graph,
				    &core->scene,
				    &core->main_camera,
				    &core->environment_probe);

		SkyboxRender(&core->render_graph,
			     FetchStandardImageView(&core->skybox_cubemap),
			     FetchStandardImageView(&core->renderer.gbuffer.depth));
	}
	RenderGraphExecuteRenderPasses(&core->render_graph, &core->render_state);
	EndGraphicsPresent(&core->render_state.cmd);
}

__declspec(dllexport) void CoreDestroy(Platform *platform_)
{
	SceneDestroy(&core->scene);
	ShadersDestroy(&core->shaders);
	AssetsDestroy(&core->assets);
	RendererDestroy(&core->renderer);
	RenderStateDestroy(&core->render_state);
	
	// ---
	
	SamplerDestroy(&core->linear_sampler);

	MeshDestroy(&core->light_sphere_mesh);
	MeshDestroy(&core->skybox_mesh);

	GPUBufferDestroy(&core->cubemap_capture_transforms);

	ImageDestroy(&core->brdf_lut_image);
	ImageDestroy(&core->skybox_cubemap);
	ImageDestroy(&core->environment_probe.irradiance);
	ImageDestroy(&core->environment_probe.prefilter);

	GraphicsDeviceDestroy();
}

__declspec(dllexport) void CoreBeforeHotReload(Platform *platform_)
{
	GraphicsDeviceBeforeHotReload();
}

__declspec(dllexport) void CoreAfterHotReload(Platform *platform_)
{
	CoreResetGlobals(platform_);
	GraphicsDeviceAfterHotReload();
}
