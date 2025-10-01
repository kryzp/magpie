#include "app.h"

#include "core/core_math.h"
#include "core/core_scratch.h"

#include "rendering/gpu_types.h"
#include "rendering/stages/stage.h"

struct app *app = NULL;
struct platform *platform = NULL;

static void camera_driver_update(struct camera_driver *driver, struct gfx_camera *camera, float dt)
{	
	if (platform->kb_pressed[KEYBOARD_KEY_tab])
		driver->active = !driver->active;

	if (!driver->active)
		return;
	
	const float mouse_deadzone = .001f;
	const float turn_speed = 1.f;
	const float move_speed = 2.5f;

	float dx = (float)(platform->mouse_position.x - platform->window_width/2);
	float dy = (float)(platform->mouse_position.y - platform->window_height/2);

	if (dx*dx + dy*dy > mouse_deadzone*mouse_deadzone) {
		driver->target_yaw   -= dx * turn_speed * dt;
		driver->target_pitch -= dy * turn_speed * dt;
	}

	driver->yaw   += (driver->target_yaw   - driver->yaw)*dt*50.f;
	driver->pitch += (driver->target_pitch - driver->pitch)*dt*50.f;

	float yaw   = driver->yaw + PIf*.5f;
	float pitch = driver->pitch;

	camera->forward = spherical_to_cartesian(1.f, yaw, pitch);

	v3 basis[3] = {0};
	basis[0] = v3_normalize(v3_cross(camera->forward, v3(0.f, 0.f, 1.f)));
	basis[1] = camera->forward;
	basis[2] = v3_normalize(v3_cross(basis[0], basis[1]));

	for (int i = 0; i < 3; i++)
		basis[i] = v3_mul_float(basis[i], move_speed * dt);
	
	if (platform->kb_down[KEYBOARD_KEY_d])
		camera->position = v3_add(camera->position, basis[0]);
	else if (platform->kb_down[KEYBOARD_KEY_a])
		camera->position = v3_sub(camera->position, basis[0]);

	if (platform->kb_down[KEYBOARD_KEY_w])
		camera->position = v3_add(camera->position, basis[1]);
	else if (platform->kb_down[KEYBOARD_KEY_s])
		camera->position = v3_sub(camera->position, basis[1]);
	
	if (platform->kb_down[KEYBOARD_KEY_space])
		camera->position = v3_add(camera->position, basis[2]);
	else if (platform->kb_down[KEYBOARD_KEY_left_shift] || platform->kb_down[KEYBOARD_KEY_right_shift])
		camera->position = v3_sub(camera->position, basis[2]);

	gfx_camera_recompute(camera);

	platform->set_mouse_position(platform->window_width / 2,
				     platform->window_height / 2);
}

static void app_fixed_update(float dt);
static void app_update(float dt);
static void app_render(struct gfx_command_buffer *cmd);

// https://songho.ca/opengl/gl_sphere.html
// TODO: Use a more efficient sphere shape like an ICOSPHERE or CUBESPHERE.
static void app_create_unit_sphere_mesh()
{
	struct scratch_arena scratch = scratch_begin(&app->permanent_arena, 1);

	u16 sector_count = 10;
	u16 stack_count  = 10;

	float sector_step = 2.f * PIf / (float)sector_count;
	float stack_step  =       PIf / (float)stack_count;

	u32 vertex_count = (stack_count + 1) * (sector_count + 1);
	u32 index_count  = (stack_count - 1) * (sector_count + 0) * 6;

	v3 *vertices = memory_arena_array(scratch.arena, vertex_count, sizeof(v3));
	u16 *indices = memory_arena_array(scratch.arena, index_count, sizeof(u16));

	u32 index = 0;

	for (int i = 0; i <= stack_count; i++) {
		float theta = PIf/2.f - i*stack_step;
		for (int j = 0; j <= sector_count; j++) {
			float phi = j * sector_step;
			vertices[index++] = spherical_to_cartesian(1.f, phi, theta);
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

	gfx_mesh_init(&app->light_sphere_mesh, &app->graphics_device,
		      sizeof(v3),
		      vertex_count, vertices,
		      index_count, indices);

	scratch_release(&scratch);
}

void app_populate_scene()
{
#if 0
	struct asset_handle damaged_helmet_model_asset_handle = asset_store_load_model(&app->assets, str8("res/DamagedHelmet/DamagedHelmet.gltf"));
	struct gfx_model *damaged_helmet_model = asset_store_model_from_handle(&app->assets, damaged_helmet_model_asset_handle);

	for (int i = 0; i < array_size(core->damaged_helmet_objects); i++) {
		for (int j = 0; j < array_size(core->damaged_helmet_objects[0]); j++) {
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

	struct gfx_light my_light = {0};
	my_light.type = LightType_Point;
	my_light.colour = v3(1.f, 1.f, 1.f);
	my_light.intensity = 0.5f;
	my_light.falloff = 0.5f;

	core->light = SceneRegisterObject(&core->scene, m4(1.f));
	SceneObjectAddLight(&core->scene, core->light, &core->render_state, &my_light);
#endif // 0
}

APP_API void app_init(struct platform *platform_)
{
	platform = platform_;
	
	struct memory_arena arena = memory_arena_init(platform->memory, platform->memory_size);
	app = memory_arena_push(&arena, sizeof(struct app));
	
	app->permanent_arena = arena;
	app->frame_arena = memory_arena_sub_arena(&app->permanent_arena, MEGABYTES(4));
	app->scene_arena = memory_arena_sub_arena(&app->permanent_arena, MEGABYTES(4));

	core_init(&app->core, &app->permanent_arena, MEGABYTES(1));

	gfx_device_init(&app->graphics_device, platform, &app->permanent_arena);
	gfx_shaders_init(&app->shaders, &app->graphics_device);
	
	asset_store_init(&app->assets, &app->permanent_arena);
	
	app->swapchain = gfx_device_swapchain_create(&app->graphics_device, platform);

	app->frame_data_buffer = gfx_device_buffer_alloc(&app->graphics_device,
							 VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
							 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
							 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							 sizeof(struct gfx_gpu_frame_data));
	
	app_create_unit_sphere_mesh();

	app->linear_sampler = gfx_device_sampler_create(&app->graphics_device, VK_FILTER_NEAREST);

	m4 capture_projection_matrix = m4_perspective(90.f, 1.f, 0.1f, 10.f);

	// Renderman introduced the left-handed Y-up cubemap in 1990
	// but we use right-handed Z-up so we have to flip these weirdly.
	m4 capture_view_matrices[] = {
		m4_lookat(v3(0.f, 0.f, 0.f), v3( 1.f,  0.f,  0.f), v3(0.f,  0.f, 1.f)), // X+
		m4_lookat(v3(0.f, 0.f, 0.f), v3(-1.f,  0.f,  0.f), v3(0.f,  0.f, 1.f)), // X-
		m4_lookat(v3(0.f, 0.f, 0.f), v3( 0.f,  0.f,  1.f), v3(0.f, -1.f, 0.f)), // Y+
		m4_lookat(v3(0.f, 0.f, 0.f), v3( 0.f,  0.f, -1.f), v3(0.f,  1.f, 0.f)), // Y-
		m4_lookat(v3(0.f, 0.f, 0.f), v3( 0.f,  1.f,  0.f), v3(0.f,  0.f, 1.f)), // Z+
		m4_lookat(v3(0.f, 0.f, 0.f), v3( 0.f, -1.f,  0.f), v3(0.f,  0.f, 1.f)), // Z-
	};

	app->cubemap_capture_transforms = gfx_device_buffer_alloc(&app->graphics_device,
								   VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
								   VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
								   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
								   sizeof(m4) * 6);

	for (int i = 0; i < 6; i++) {
		m4 m = m4_mul_m4(capture_projection_matrix, capture_view_matrices[i]);
		gfx_buffer_write(&app->cubemap_capture_transforms,
				 &m,
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

	gfx_mesh_init(&app->skybox_mesh, &app->graphics_device,
		      sizeof(v3),
		      array_size(vertices), vertices,
		      array_size(indices), indices);

	gfx_gbuffer_init(&app->gbuffer, &app->graphics_device, &app->swapchain);

	gfx_render_scene_init(&app->render_scene, &app->graphics_device, &app->permanent_arena);
	
	app->main_camera = gfx_camera_init_perspective(v3(0.f, 0.f, 0.f),
						       v3(0.f, 1.f, 0.f),
						       100.f,
						       1280.f / 720.f,
						       .1f, 20.f);

	app->lighting_attachment = gfx_device_texture_alloc_2d_rw(&app->graphics_device,
								  app->swapchain.width, app->swapchain.height,
								  VK_FORMAT_R32G32B32A32_SFLOAT, 1);

	gfx_render_graph_init(&app->render_graph);

	struct asset_handle environment_asset_handle = asset_store_load_texture(&app->assets, &app->graphics_device, str8("res/environment_map.hdr"));
	
	app->skybox_cubemap = gfx_device_texture_alloc_cubemap(&app->graphics_device,
							       1024, VK_FORMAT_R32G32B32A32_SFLOAT, 4);

	struct stage_hdr_texture_to_cubemap_input hdr_input = {0};
	
	hdr_input.texture = gfx_device_texture_view_fetch_std(&app->graphics_device, asset_store_texture_from_handle(&app->assets, environment_asset_handle));
	hdr_input.output = gfx_device_texture_view_fetch_std(&app->graphics_device, &app->skybox_cubemap);

	stage_add_hdr_texture_to_cubemap(&app->render_graph, &hdr_input);

	app->brdf_lut_texture = gfx_device_texture_alloc_2d(&app->graphics_device,
							    512, 512, VK_FORMAT_R32G32_SFLOAT, 1);

	app_populate_scene();

	timer_start(&app->global_timer);
	timer_start(&app->delta_timer);
}

APP_API void app_destroy(struct platform *platform_)
{
	gfx_device_wait_idle(&app->graphics_device);

	gfx_device_buffer_destroy(&app->graphics_device, &app->frame_data_buffer);
	gfx_device_buffer_destroy(&app->graphics_device, &app->cubemap_capture_transforms);

	gfx_gbuffer_destroy(&app->gbuffer, &app->graphics_device);

	gfx_device_texture_destroy(&app->graphics_device, &app->lighting_attachment);
	gfx_device_texture_destroy(&app->graphics_device, &app->brdf_lut_texture);
	gfx_device_texture_destroy(&app->graphics_device, &app->skybox_cubemap);
	
	gfx_environment_probe_destroy(&app->environment_probe, &app->graphics_device);

	gfx_device_sampler_destroy(&app->graphics_device, &app->linear_sampler);

	gfx_mesh_destroy(&app->light_sphere_mesh, &app->graphics_device);
	gfx_mesh_destroy(&app->skybox_mesh, &app->graphics_device);
	
	asset_store_destroy(&app->assets, &app->graphics_device);
	
	gfx_render_scene_destroy(&app->render_scene, &app->graphics_device);
	gfx_shaders_destroy(&app->shaders, &app->graphics_device);
	gfx_render_graph_destroy(&app->render_graph);
	gfx_device_swapchain_destroy(&app->graphics_device, &app->swapchain);
	gfx_device_destroy(&app->graphics_device, platform);
}

APP_API void app_hot_load(struct platform *platform_)
{
	platform = platform_;
	app = platform->memory;
	
	core_hot_load(&app->core);
	gfx_device_hot_load(&app->graphics_device);
}

APP_API void app_hot_unload(struct platform *platform_)
{
	gfx_device_hot_unload(&app->graphics_device);
}

APP_API bool app_tick(struct platform *platform_)
{
	memory_arena_clear(&app->frame_arena);
	
	if (platform->kb_pressed[KEYBOARD_KEY_escape]) {
		debug_log("Quitting...");
		return true;
	}

	const float dt = timer_reset(&app->delta_timer);
	const float fixed_dt = 1.f / (float)(platform->target_fps);
	
	app_update(dt);

	app->delta_accumulator += min_value(dt, fixed_dt);

	while (app->delta_accumulator >= fixed_dt) {
		app_fixed_update(fixed_dt);
		app->delta_accumulator -= fixed_dt;
	}
	
	struct gfx_command_buffer present_cmd = gfx_device_begin_present(&app->graphics_device, &app->swapchain);
	app_render(&present_cmd);
	gfx_device_end_present(&app->graphics_device, &app->swapchain, &present_cmd);

	return false;
}

static void app_fixed_update(float dt)
{
}

static void app_update(float dt)
{
	float t = timer_elapsed_seconds(&app->global_timer);
	
	struct gfx_render_scene *scene = &app->render_scene;
	struct gfx_camera *camera = &app->main_camera;

	camera_driver_update(&app->main_camera_driver, camera, 1.f / 120.f);
	
#if 0
	for (u32 i = 0; i < array_size(app->damaged_helmet_objects); i++) {
		for (u32 j = 0; j < array_size(app->damaged_helmet_objects[0]); j++) {
			float d = square_root(i*i + j*j);
			SceneObjectFromHandle(scene, core->damaged_helmet_objects[i][j])
				->transform = M4Transform(v3(j, i, 0.f),
							  QuatInitEuler(0.f, t+d, 0.f),
							  v3u(1.f),
							  v3u(0.f));
		}
	}

	v3 position = v3_sub_v3(camera->position, V3Multiplyfloat(camera->forward, camera->position.z / camera->forward.z));
	position.z = 1.f;
	
	SceneObjectFromHandle(scene, core->light)
		->transform = M4Transform(position,
					  QuatInitIdentity(),
					  v3u(1.f),
					  v3u(0.f));
#endif // 0
	
	gfx_render_scene_resolve_removing(scene);

	gfx_render_scene_update(&app->render_scene, &app->graphics_device);
	
	gfx_render_graph_update(&app->render_graph);
}

static void app_frame_data_upload()
{
	struct gfx_camera *camera = &app->main_camera;
	
	struct gfx_gpu_frame_data frame_data = {0};
	frame_data.view = camera->view;
	frame_data.projection = camera->projection;
	frame_data.view_projection = m4_mul_m4(frame_data.projection, frame_data.view);
	frame_data.view_projection_no_translation = m4_mul_m4(frame_data.projection, m4_remove_translation(frame_data.view));
	frame_data.inv_view = m4_inverse(frame_data.view);
	frame_data.inv_projection = m4_inverse(frame_data.projection);
	frame_data.camera_position = camera->position;
	frame_data.window_resolution.x = platform->window_pixel_width;
	frame_data.window_resolution.y = platform->window_pixel_height;
	frame_data.time = timer_elapsed_seconds(&app->global_timer);

	gfx_buffer_write(&app->frame_data_buffer, &frame_data,
			 sizeof(struct gfx_gpu_frame_data), 0);
}

static void app_render(struct gfx_command_buffer *cmd)
{
	app_frame_data_upload();

	/*
	struct stage_render_scene_input render_scene_input = {
		.clear_indirect_buffer = NULL,
		.draw_indirect_buffer = NULL,
		.batch_count = NULL
	};
	*/
	
	struct stage_skybox_input skybox_input = {
		.skybox = gfx_device_texture_view_fetch_std(&app->graphics_device, &app->skybox_cubemap),
		.target = gfx_device_texture_view_fetch_std(&app->graphics_device, &app->lighting_attachment),
		.depth = app->gbuffer.depth_view,
		.frame_data_buffer = &app->frame_data_buffer
	};
	
	struct stage_post_processing_input pp_input = {
		.input = gfx_device_texture_view_fetch_std(&app->graphics_device, &app->lighting_attachment),
		.output = gfx_device_texture_view_fetch_std(&app->graphics_device, &app->lighting_attachment),
		.exposure = 1.15f
	};
	
	struct stage_blit_lighting_to_swapchain_input blts_input = {
		.lighting = gfx_device_texture_view_fetch_std(&app->graphics_device, &app->lighting_attachment),
		.swapchain = gfx_swapchain_current_view(&app->swapchain)
	};

	//stage_add_render_scene                 (&app->render_graph, &render_scene_input);
	//stage_add_clear_draw_indirect          (&app->render_graph, &clear_draw_input);
	stage_add_skybox                       (&app->render_graph, &skybox_input);
	stage_add_post_processing              (&app->render_graph, &pp_input);
	stage_add_blit_lighting_to_swapchain   (&app->render_graph, &blts_input);
	
	struct gfx_render_stage present_stage = {0};
	gfx_render_stage_init(&present_stage, GFX_RENDER_STAGE_present);
	gfx_render_graph_push(&app->render_graph, &present_stage);
	
	gfx_render_graph_render(&app->render_graph,
				&app->graphics_device,
				&app->swapchain,
				cmd);
	
	gfx_render_graph_reset(&app->render_graph);
}
