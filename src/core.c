// TODO: (In order of what to do next to achieve feature parity with magpie C++)
//       [x]  1. Pipeline state caching, two seperate hash
//               tables for layouts and pipelines. Also cache
//               image views automaticlly.
//       [x]  2. Irradiance + Prefilter map generation.
//       [x]  3. Remove combined image-sampler from bindless
//               and add seperate image + sampler tables.
//       [x]  4. Generic hash table implementation.
//       [x]  5. Well commented codebase (self commenting code counts).
//       [x]  6. More Assert(...), DebugLog(...) and DebugLogCrash(...) in the codebase.
//       [x]  7. Investigate how I'm taking up ~100kb of memory in the allocated 32MB?
//               --> RenderPass is just a very big struct, and the renderer has 32
//                   of them at all times.
//       [x]  8. Model loading.
//       [x]  9. Split up files accordingly to make the code easier to manage.
//       [x] 10. Material system.
//       [x] 11. Deferred Rendering.
//       [x] 12. Scene.
//               --> Camera, lights, etc...
//       [x] 13. Final Requirements
//               [x] Asset system to stop unfreed images when loading in models.
//               [x] Shaders should automatically assign their push constants sizes
//                   rather than me having to do it manually.
//               [x] Move render graph into core and command buffer into render context.
//               [x] RenderState shouldn't have any associated functions like
//                   RenderStateGenerateBRDFLookUp(...), rather there should be seperate
//                   renderers for that sort of thing.
//                   --> Same goes for e.g: updating per frame data.
//               [x] Proper layout transitions and synchronization in render graph.
//               [x] GPU driven rendering.
//               [x] Use multiple sets for bindless.
//                   --> This way we can use DESCRIPTOR_VARIABLE_COUNT, instead of
//                       a fixed size set of 256.
//
//               <<< MAGPIE C++ ENDS HERE >>>
//
//       [ ] 14. Debug renderer (lines, spheres, etc...) (seperate thing)
//       [ ] 15. Text rendering (fonts)
//       [ ] 16. (This applies to all bindless resources.)
//               resource_id should *not* be assigned in the
//               graphics device. In fact, the graphics device
//               should not be managing bindless in the first
//               place, that should be a policy of the renderer.
//               --> Maybe in the future, have a BindlessResources
//                   struct in the high level, that different renderers
//                   can use to manage their bindless resources
//       [ ] 17. Switch to using timeline semaphores over fences for frame synchronisation.
//       [ ] 18. Start going through ideas list in readme.md.

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "ext/spirv_reflect.c"

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
#include "graphics.h"
#include "model.h"
#include "assets.h"
#include "camera.h"
#include "scene.h"
#include "render_graph.h"
#include "mesh_pass.h"
#include "gpu_types.h"
#include "render_state.h"
#include "deferred.h"
#include "shaders.h"
#include "core.h"
#include "scratch.h"

#include "timer.c"
#include "scratch.c"
#include "hash_table.c"
#include "graphics.c"
#include "shaders.c"
#include "model.c"
#include "assets.c"
#include "render_graph.c"
#include "render_state.c"
#include "mesh_pass.c"
#include "camera.c"
#include "scene.c"
#include "deferred.c"
#include "skybox.c"
#include "ibl_renderer.c"
#include "brdf_lut.c"
#include "frustum_culling.c"
#include "post_processing.c"

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

	core->light_sphere_mesh = MeshInit(sizeof(v3),
					   vertex_count, vertices,
					   index_count, indices);

	ReleaseScratch(&scratch);
}

internal void CoreLightingAttachmentBlitToSwapchain(RenderState *rs, void *context)
{
	Image *src = &core->lighting_attachment;
	Image *dst = SwapchainCurrentImage(&graphics_device->swapchain);
	
	VkImageBlit region = {0};
	
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel = 0;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount = 1;
	region.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
	region.srcOffsets[1] = (VkOffset3D){ src->width, src->height, 1 };

	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.mipLevel = 0;
	region.dstSubresource.baseArrayLayer = 0;
	region.dstSubresource.layerCount = 1;
	region.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
	region.dstOffsets[1] = (VkOffset3D){ dst->width, dst->height, 1 };

	CmdBlitImage(&rs->cmd,
		     src, dst,
		     1, &region,
		     VK_FILTER_LINEAR);
}

internal CoreFrameData *CoreCurrentFrame()
{
	return core->per_frame_data + graphics_device->current_frame_index;
}

internal void CoreCreatePerFrameObjects()
{
	CoreFrameData *frame = core->per_frame_data;
	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++, frame++) {
		frame->frame_data_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
							  VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
							  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							  sizeof(GPU_FrameData));
		
		frame->object_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
						      VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
						      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						      sizeof(GPU_ObjectData) * SCENE_MAX_OBJECTS);

		frame->light_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
						     VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
						     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						     sizeof(GPU_Light) * SCENE_MAX_OBJECTS);
	}
}

internal void CoreDestroyPerFrameObjects()
{
	CoreFrameData *frame = core->per_frame_data;
	for (i32 i = 0; i < FRAMES_IN_FLIGHT; i++, frame++) {
		GPUBufferDestroy(&frame->frame_data_buffer);
		GPUBufferDestroy(&frame->object_buffer);
		GPUBufferDestroy(&frame->light_buffer);
	}
}

internal void CoreResetGlobals(Platform *platform_)
{
	platform = platform_;
	core = platform->permanent_memory;
	graphics_device = &core->graphics_device;
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

	core->cubemap_capture_transforms = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
							  VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
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

	core->skybox_mesh = MeshInit(sizeof(v3),
				     ArraySize(vertices), vertices,
				     ArraySize(indices), indices);

	CoreCreatePerFrameObjects();

	RenderStateInit(&core->render_state, &core->material_buffer);
	
	core->material_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
					       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
					       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
					       sizeof(GPU_Material) * SCENE_MAX_MATERIALS);

	core->compacted_instance_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
							 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							 sizeof(u32) * SCENE_MAX_OBJECTS);

	core->instance_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
					       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
					       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
					       sizeof(GPU_Instance) * SCENE_MAX_OBJECTS);
	
	core->draw_indirect_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
						    VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
						    VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
						    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						    sizeof(GPU_Indirect) * SCENE_MAX_OBJECTS);
	
	core->clear_indirect_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
						     VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
						     VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
						     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
						     sizeof(GPU_Indirect) * SCENE_MAX_OBJECTS);

	core->instance_buffer_dirty = true;
	core->indirect_buffer_dirty = true;
	
	GBufferInit(&core->gbuffer);

	core->lighting_attachment = ImageAlloc2D_RW(graphics_device->swapchain.width, graphics_device->swapchain.height,
						    VK_FORMAT_R32G32B32A32_SFLOAT, 1);
	
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

	for (u32 i = 0; i < ArraySize(core->damaged_helmet_objects); i++) {
		for (u32 j = 0; j < ArraySize(core->damaged_helmet_objects[0]); j++) {
			core->damaged_helmet_objects[i][j] = SceneRegisterObject(&core->scene, m4(1.f));
			SceneObjectAddMesh(&core->scene,
					   core->damaged_helmet_objects[i][j],
					   &core->render_state,
					   &core->assets,
					   &damaged_helmet_model->sub_models[0].mesh,
					   &damaged_helmet_model->sub_models[0].material,
					   SceneObjectFlag_DrawDeferredPass);
		}
	}

	Light my_light = {0};
	my_light.type = LightType_Point;
	my_light.colour = v3(1.f, 1.f, 1.f);
	my_light.intensity = 0.5f;
	my_light.falloff = 0.5f;

	core->light = SceneRegisterObject(&core->scene, m4(1.f));
	SceneObjectAddLight(&core->scene, core->light, &core->render_state, &my_light);

	// ---
	
	core->starting_ticks = platform->GetPerformanceCounter();

	TimerStart(&core->delta_timer);
}

internal void CoreFixedUpdate(f32 dt)
{
}

internal void CoreUpdate(f32 dt)
{
	f32 t = GetTotalElapsedSecondsF();

	Scene *scene = &core->scene;
	Camera *camera = &core->main_camera;

	CameraDriverUpdate(&core->main_camera_driver, camera, 1.f / 120.f);
	
	for (u32 i = 0; i < ArraySize(core->damaged_helmet_objects); i++) {
		for (u32 j = 0; j < ArraySize(core->damaged_helmet_objects[0]); j++) {
			f32 d = SquareRoot(i*i + j*j);
			SceneObjectFromHandle(scene, core->damaged_helmet_objects[i][j])
				->transform = M4Transform(v3(j, i, 0.f),
							  QuatInitEuler(0.f, t+d, 0.f),
							  v3u(1.f),
							  v3u(0.f));
		}
	}

	v3 position = V3SubV3(camera->position, V3MultiplyF32(camera->forward, camera->position.z / camera->forward.z));
	position.z = 1.f;
	
	SceneObjectFromHandle(scene, core->light)
		->transform = M4Transform(position,
					  QuatInitIdentity(),
					  v3u(1.f),
					  v3u(0.f));

	SceneResolveRemoving(scene);
}

internal void CoreFrameDataUploadPerFrameBuffer(CoreFrameData *frame, Camera *camera)
{
	GPU_FrameData frame_data = {0};
	frame_data.view = camera->view;
	frame_data.projection = camera->projection;
	frame_data.view_projection = M4MultiplyM4(frame_data.projection, frame_data.view);
	frame_data.view_projection_no_translation = M4MultiplyM4(frame_data.projection, M4RemoveTranslation(frame_data.view));
	frame_data.inv_view = M4Inverse(frame_data.view);
	frame_data.inv_projection = M4Inverse(frame_data.projection);
	frame_data.camera_position = camera->position;
	frame_data.window_resolution.x = platform->window_pixel_width;
	frame_data.window_resolution.y = platform->window_pixel_height;
	frame_data.time = GetTotalElapsedSecondsF();

	GPUBufferWrite(&frame->frame_data_buffer,
		       &frame_data,
		       sizeof(GPU_FrameData), 0);
}

// TODO: This is kind of a placeholder.
//       At least, the code is pretty bad.
//       --> Shouldn't be calling GPUBufferWrite() for every
//           object individually, at the very least.
internal void CoreFrameDataUploadObjects(CoreFrameData *frame, RenderState *rs, Scene *scene)
{
	u32 mesh_count = 0;
	u32 light_count = 0;
	
	for (SceneObject *object = scene->objects; object; object = object->next) {

		// If the object has a mesh.
		if (object->mesh_id != SCENE_INVALID_HANDLE) {
			GPU_ObjectData object_data = {0};
			object_data.model_matrix = object->transform;
			object_data.normal_matrix = M4Inverse(M4Transpose(object->transform));

			GPUBufferWrite(&frame->object_buffer,
				       &object_data,
				       sizeof(GPU_ObjectData),
				       sizeof(GPU_ObjectData) * mesh_count);

			mesh_count++;
		}

		// If the object has a light.
		if (object->light_id != SCENE_INVALID_HANDLE) {
			Light *light = &rs->lights[object->light_id];
			
			f32 epsilon_intensity = .1f;
			f32 light_max = V3MaxValue(light->colour);
			f32 heuristic_radius = SquareRoot((light->intensity * light_max) / (light->falloff * epsilon_intensity));

			GPU_Light gpu_light = {0};
			gpu_light.position     = object->transform.c[3]; // Last column of transformation matrix is translation.
			gpu_light.colour.xyz   = light->colour;
			gpu_light.colour.w     = light->intensity;
			gpu_light.attenuation  = v4(light->falloff, 0.f, 0.f, 0.f);
			gpu_light.transform    = M4Transform(gpu_light.position.xyz,
							     QuatInitIdentity(),
							     v3u(heuristic_radius),
							     v3u(0.f));

			GPUBufferWrite(&frame->light_buffer,
				       &gpu_light,
				       sizeof(GPU_Light),
				       sizeof(GPU_Light) * light_count);

			light_count++;
		}
	}
}

internal void CoreClearDrawIndirectBuffer(CommandBuffer *cmd, MeshPass *mesh_pass)
{
	VkBufferCopy indirect_region = {0};
	indirect_region.srcOffset = 0;
	indirect_region.dstOffset = 0;
	indirect_region.size = mesh_pass->batch_count * sizeof(GPU_Indirect);
	
	CmdCopyBufferToBuffer(cmd,
			      &core->clear_indirect_buffer,
			      &core->draw_indirect_buffer,
			      1, &indirect_region);
}

internal void CoreRender()
{
	Scene *scene = &core->scene;
	RenderState *rs = &core->render_state;
	CoreFrameData *frame = CoreCurrentFrame();
	
	rs->cmd = GraphicsBeginPresent();

	CoreFrameDataUploadPerFrameBuffer(frame, &core->main_camera);
	CoreFrameDataUploadObjects(frame, rs, scene);

	RenderStateMergeMeshes(rs);

	// TODO: Only bother recomputing mesh_pass (and its derivatives)
	//       when the scene actually changes.
	MeshPass mesh_pass = {0};
	MeshPassPopulate(&mesh_pass, &core->frame_arena, rs, scene);

	if (core->instance_buffer_dirty) {
		GPU_Instance *instances_array = GPUBufferData(&core->instance_buffer);
		RenderStateFillInstancesArray(rs, &mesh_pass, instances_array);
		core->instance_buffer_dirty = false;
	}
	
	if (core->indirect_buffer_dirty) {
		GPU_Indirect *indirect_array = GPUBufferData(&core->clear_indirect_buffer);
		RenderStateFillIndirectArray(rs, &mesh_pass, indirect_array);
		core->indirect_buffer_dirty = false;
	}
	
	CoreClearDrawIndirectBuffer(&rs->cmd, &mesh_pass);
	
	// ---

	struct frustum_culling_input frustum_culling_input = {0};
	frustum_culling_input.camera          = &core->main_camera;
	frustum_culling_input.mesh_pass       = &mesh_pass;
	frustum_culling_input.instance_buffer = &core->instance_buffer;
	frustum_culling_input.indirect_buffer = &core->draw_indirect_buffer;
	frustum_culling_input.output_buffer   = &core->compacted_instance_buffer;
	
	ComputeFrustumCulling(&core->render_graph, &frustum_culling_input);
	
	struct deferred_renderer_input deferred_renderer_input = {0};
	deferred_renderer_input.scene             = &core->scene;
	deferred_renderer_input.camera            = &core->main_camera;
	deferred_renderer_input.probe             = &core->environment_probe;
	deferred_renderer_input.mesh_pass         = &mesh_pass;
	deferred_renderer_input.frame_data_buffer = &frame->frame_data_buffer;
	deferred_renderer_input.object_buffer     = &frame->object_buffer;
	deferred_renderer_input.instance_buffer   = &core->compacted_instance_buffer;
        deferred_renderer_input.indirect_buffer   = &core->draw_indirect_buffer;
	deferred_renderer_input.light_buffer      = &frame->light_buffer;
	deferred_renderer_input.gbuffer           = &core->gbuffer;
	deferred_renderer_input.lighting          = FetchStandardImageView(&core->lighting_attachment);
	
	DeferredRenderFrame(&core->render_graph, &deferred_renderer_input);
	
	struct skybox_renderer_input skybox_renderer_input = {0};
	skybox_renderer_input.skybox            = FetchStandardImageView(&core->skybox_cubemap);
	skybox_renderer_input.frame_data_buffer = &frame->frame_data_buffer;
	skybox_renderer_input.target            = FetchStandardImageView(&core->lighting_attachment);
	skybox_renderer_input.depth             = FetchStandardImageView(&core->gbuffer.depth);
	
	SkyboxRender(&core->render_graph, &skybox_renderer_input);

	struct post_processing_input post_processing_input = {0};
	post_processing_input.exposure = 1.15f;
	post_processing_input.input = FetchStandardImageView(&core->lighting_attachment);
	post_processing_input.output = FetchStandardImageView(&core->lighting_attachment);

	PostProcessingPass(&core->render_graph, &post_processing_input);
	
	// ---

	RenderPass lighting_to_swapchain_pass = {0};
	lighting_to_swapchain_pass.type = RenderPassType_Transfer;
	lighting_to_swapchain_pass.transfer.Record = CoreLightingAttachmentBlitToSwapchain;
	lighting_to_swapchain_pass.transfer.src_count = 1;
	lighting_to_swapchain_pass.transfer.src[0] = FetchStandardImageView(&core->lighting_attachment);
	lighting_to_swapchain_pass.transfer.dst_count = 1;
	lighting_to_swapchain_pass.transfer.dst[0] = SwapchainCurrentImageView(&graphics_device->swapchain);
	
	RenderGraphPush(&core->render_graph, &lighting_to_swapchain_pass);
	
	RenderPass present_pass = {0};
	present_pass.type = RenderPassType_Present;
	present_pass.present.swapchain = SwapchainCurrentImage(&graphics_device->swapchain);

	RenderGraphPush(&core->render_graph, &present_pass);
	
	// ---
	
	RenderGraphExecute(&core->render_graph, &core->render_state, &core->frame_arena);
	GraphicsEndPresent(&core->render_state.cmd);
}

__declspec(dllexport) void CoreTick(Platform *platform_)
{
	MemoryArenaClear(&core->frame_arena);
	MemoryArenaClear(&core->scratch_arenas[0]);
	MemoryArenaClear(&core->scratch_arenas[1]);

	if (platform->kb_pressed[KeyboardKey_Escape]) {
		DebugLog("Quitting...");
		platform->exit = true;
	}

	f32 dt = TimerResetF(&core->delta_timer);
	const f32 fixed_dt = 1.f / (f32)(platform->target_fps);
	
	CoreUpdate(dt);

	core->delta_accumulator += MinValue(dt, fixed_dt);
	
	while (core->delta_accumulator >= fixed_dt) {
		CoreFixedUpdate(fixed_dt);
		core->delta_accumulator -= fixed_dt;
	}
	
	CoreRender();
}

__declspec(dllexport) void CoreDestroy(Platform *platform_)
{
	CoreDestroyPerFrameObjects();
	
	GBufferDestroy(&core->gbuffer);
	ImageDestroy(&core->lighting_attachment);

	GPUBufferDestroy(&core->material_buffer);
	GPUBufferDestroy(&core->cubemap_capture_transforms);

	GPUBufferDestroy(&core->compacted_instance_buffer);
	GPUBufferDestroy(&core->instance_buffer);
	GPUBufferDestroy(&core->draw_indirect_buffer);
	GPUBufferDestroy(&core->clear_indirect_buffer);

	SamplerDestroy(&core->linear_sampler);

	MeshDestroy(&core->light_sphere_mesh);
	MeshDestroy(&core->skybox_mesh);

	ImageDestroy(&core->brdf_lut_image);
	ImageDestroy(&core->skybox_cubemap);
	ImageDestroy(&core->environment_probe.irradiance);
	ImageDestroy(&core->environment_probe.prefilter);

	// ---
	
	SceneDestroy(&core->scene);
	ShadersDestroy(&core->shaders);
	AssetsDestroy(&core->assets);
	RenderStateDestroy(&core->render_state);

	// ---
	
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
