#define VOLK_IMPLEMENTATION
#include "ext/volk.h"
#include "ext/vk_mem_alloc.h"
#include "ext/spirv_reflect.c"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "kp.h"

#define STBI_ASSERT Assert
#define STBIW_ASSERT Assert

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

#include "platform.h"

#include "arena.h"
#include "arena.c"

#include "timer.h"
#include "hash_table.h"
#include "graphics_device.h"
#include "model.h"
#include "assets.h"
#include "scene.h"
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
	u16 stack_count  = 10;

	f32 sector_step = 2.f * PIf / (f32)sector_count;
	f32 stack_step  =       PIf / (f32)stack_count;

	u32 vertex_count = (stack_count + 1) * (sector_count + 1);
	u32 index_count  = (stack_count - 1) * (sector_count + 0) * 6;

	v3 *vertices = MemoryArenaPush(scratch.arena, sizeof(v3)  * vertex_count);
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
		u16 k1 = (sector_count + 1) * (i + 0); // Current stack.
		u16 k2 = (sector_count + 1) * (i + 1); // Next stack.

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

internal CoreFrameData *CoreCurrentFrame()
{
	return core->per_frame_data + graphics_device->current_frame_index;
}

internal void CoreCreatePerFrameObjects()
{
	CoreFrameData *frame = core->per_frame_data;
	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++, frame++) {
		frame->frame_data_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
							  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
							  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							  sizeof(GPU_FrameData));
		
		frame->object_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
						      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						      sizeof(GPU_ObjectData) * SCENE_MAX_OBJECTS);

		frame->light_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
						     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
						     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						     sizeof(GPU_Light) * SCENE_MAX_OBJECTS);

		frame->indirect_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
							VK_BUFFER_USAGE_TRANSFER_DST_BIT |
							VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
							VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							sizeof(VkDrawIndexedIndirectCommand) *
							SCENE_MAX_OBJECTS);
	}
}

internal void CoreDestroyPerFrameObjects()
{
	CoreFrameData *frame = core->per_frame_data;
	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++, frame++) {
		GPUBufferDestroy(&frame->frame_data_buffer);
		GPUBufferDestroy(&frame->object_buffer);
		GPUBufferDestroy(&frame->light_buffer);
		GPUBufferDestroy(&frame->indirect_buffer);
	}
}

internal void CoreResetGlobals(Platform *platform_)
{
	platform = platform_;
	core = platform->permanent_memory;
	graphics_device = &core->graphics_device;
	vertex_formats = &core->vertex_formats;
	shaders = &core->shaders;
}

internal void CoreInitArenas()
{
	// This is a bit fiddly, but we essentially create a temporary arena,
	// around the permanent memory and allocate core onto it, then set the
	// cores arena to that arena. So core maintains an arena that contains itself.
	// --> I'd say thats pretty neat. :)
	MemoryArena tmp = MemoryArenaInit(platform->permanent_memory, platform->permanent_memory_size);
	core = MemoryArenaPush(&tmp, sizeof(Core));
	core->permanent_arena = tmp;

	// Frame arena is just the transient memory.
	core->frame_arena = MemoryArenaInit(platform->transient_memory, platform->transient_memory_size);

	// Scene arena is a section of the permanent memory arena.
	core->scene_arena = MemoryArenaSubArena(&core->permanent_arena, Megabytes(4));

	// Scratch arenas are allocated onto the scratch memories.
	core->scratch_arenas[0] = MemoryArenaInit(platform->scratch_memory[0], platform->scratch_memory_size);
	core->scratch_arenas[1] = MemoryArenaInit(platform->scratch_memory[1], platform->scratch_memory_size);
}

__declspec(dllexport) void CoreInit(Platform *platform_)
{
	CoreResetGlobals(platform_);

	CoreInitArenas();

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

	CoreCreatePerFrameObjects();

	core->material_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
					       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
					       sizeof(GPU_Material) * SCENE_MAX_MATERIALS);

	RenderStateInit(&core->render_state, &core->material_buffer);
	
	RendererInit(&core->renderer);
	
	u32 environment_asset_handle = AssetsLoadTexture(&core->assets, str8("res/environment_map.hdr"));

	core->skybox_cubemap = ImageAllocCubemap(1024, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
	EnvironmentMapFromHDR(&core->render_graph, &core->skybox_cubemap, AssetsImageFromHandle(&core->assets, environment_asset_handle));

	core->brdf_lut_image = ImageAlloc2D(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);
	BRDFLookUpGenerate(&core->render_graph, &core->brdf_lut_image);

	core->environment_probe = EnvironmentProbeInit();
	
	struct ibl_renderer_input ibl_renderer_input = {0};
	ibl_renderer_input.probe = &core->environment_probe;
	ibl_renderer_input.skybox = FetchStandardImageView(&core->skybox_cubemap);
	
	IBLRendererGenerateEnvironmentProbe(&core->render_graph, &ibl_renderer_input);

	core->main_camera = CameraInitPerspective(v3(0.f, 0.f, 0.f),
						  v3(0.f, 1.f, 0.f),
						  100.f,
						  1280.f / 720.f,
						  .1f, 20.f);
	
	SceneInit(&core->scene, &core->scene_arena);

	u32 damaged_helmet_model_asset_handle = AssetsLoadModel(&core->assets, str8("res/DamagedHelmet/DamagedHelmet.gltf"));
	Model *damaged_helmet_model = AssetsModelFromHandle(&core->assets, damaged_helmet_model_asset_handle);

	for (i32 i = 0; i < ArraySize(core->damaged_helmet_objects); i++) {
		core->damaged_helmet_objects[i] = SceneRegisterObject(&core->scene, m4(1.f));
		SceneObjectAddMesh(&core->scene,
				   core->damaged_helmet_objects[i],
				   &core->render_state,
				   &core->assets,
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

internal void _CoreUpdate()
{
	f32 t = GetTotalElapsedSecondsF();

	Scene *scene = &core->scene;
	Camera *camera = &core->main_camera;
	
	// Camera.
	camera->position = v3(0.f, -2.f, 0.f);
	camera->forward = v3(0.f, 1.f, 0.f);

	CameraRecompute(camera);

	// Scene.
	for (i32 i = 0; i < ArraySize(core->damaged_helmet_objects); i++) {
		SceneObjectFromHandle(scene, core->damaged_helmet_objects[i])
			->transform = M4Transform(v3(i, 2.f, .4f * SinF(t)),
						  QuatInitEuler(0.f, t, 0.f),
						  v3u(1.f),
						  v3u(0.f));
	}

	SceneObjectFromHandle(scene, core->light)->transform = M4Transform(v3(SinF(t), 2.f, 1.f),
										  QuatInitIdentity(),
										  v3u(1.f),
										  v3u(0.f));

	SceneResolveRemoving(scene);
}

internal void CorePopulateRenderData()
{
	CoreFrameData *current_frame = CoreCurrentFrame();
	Scene *scene = &core->scene;
	Camera *camera = &core->main_camera;
	RenderState *rs = &core->render_state;
	
	RenderStateMergeMeshes(rs);
	PopulateMeshPass(&core->mesh_pass, rs, &core->frame_arena, scene);
	
	u32 mesh_count = 0;
	u32 light_count = 0;
	
	for (SceneObject *object = scene->objects; object; object = object->next) {

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
			command.indexCount = rs->meshes[object->mesh_id].index_count;
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
			Light *light = &rs->lights[object->light_id];
			
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

			GPUBufferWrite(&current_frame->light_buffer,
				       &gpu_light,
				       sizeof(GPU_Light),
				       sizeof(GPU_Light) * light_count);

			light_count++;
		}
	}

	// Per-frame buffer
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

	GPUBufferWrite(&current_frame->frame_data_buffer,
		       &frame_data,
		       sizeof(GPU_FrameData), 0);
}

internal void _CoreRender()
{
	core->render_state.cmd = BeginGraphicsPresent();

	CorePopulateRenderData();

	// ---
	
	struct deferred_renderer_input deferred_renderer_input = {0};
	deferred_renderer_input.scene     = &core->scene;
	deferred_renderer_input.camera    = &core->main_camera;
	deferred_renderer_input.probe     = &core->environment_probe;
	deferred_renderer_input.mesh_pass = &core->mesh_pass;
	deferred_renderer_input.target    = GetCurrentSwapchainImageView(&graphics_device->swapchain);

	DeferredRenderFrame(&core->renderer, &core->render_graph,
			    &deferred_renderer_input);

	struct skybox_renderer_input skybox_renderer_input = {0};
	skybox_renderer_input.skybox = FetchStandardImageView(&core->skybox_cubemap);
	skybox_renderer_input.target = GetCurrentSwapchainImageView(&graphics_device->swapchain);
	skybox_renderer_input.depth  = FetchStandardImageView(&core->renderer.gbuffer.depth);
		
	SkyboxRender(&core->render_graph,
		     &skybox_renderer_input);

	// ---
	
	RenderGraphExecuteRenderPasses(&core->render_graph, &core->render_state);
	EndGraphicsPresent(&core->render_state.cmd);
}

__declspec(dllexport) void CoreUpdate(Platform *platform_)
{
	MemoryArenaClear(&core->frame_arena);
	MemoryArenaClear(&core->scratch_arenas[0]);
	MemoryArenaClear(&core->scratch_arenas[1]);

	if (platform->kb_pressed[KeyboardKey_Escape]) {
		DebugLog("Quitting...");
		platform->exit = true;
	}

	_CoreUpdate();
	_CoreRender();
}

__declspec(dllexport) void CoreDestroy(Platform *platform_)
{
	SceneDestroy(&core->scene);
	ShadersDestroy(&core->shaders);
	AssetsDestroy(&core->assets);
	RendererDestroy(&core->renderer);
	RenderStateDestroy(&core->render_state);
	
	// ---

	CoreDestroyPerFrameObjects();
	
	GPUBufferDestroy(&core->material_buffer);
	GPUBufferDestroy(&core->cubemap_capture_transforms);

	SamplerDestroy(&core->linear_sampler);

	MeshDestroy(&core->light_sphere_mesh);
	MeshDestroy(&core->skybox_mesh);

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
