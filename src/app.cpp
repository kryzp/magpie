#include "app.h"

#include "platform/platform.h"
#include "core/scratch.h"
#include "math/calc.h"

#include "assets/model_serializer.h"

void CameraDriver::update(gfx::Camera &camera, const Platform &platform, float dt)
{
	if (!active)
		return;
	
	const float mouse_deadzone = .001f;
	const float turn_speed = 1.f;
	const float move_speed = 5000.f;

	float dx = (float)(platform.mouse_position.x - platform.window_width/2);
	float dy = (float)(platform.mouse_position.y - platform.window_height/2);

	if (dx*dx + dy*dy > mouse_deadzone*mouse_deadzone) {
		target_yaw -= dx * turn_speed * 3500.f * dt;
		target_pitch -= dy * turn_speed * 3500.f * dt;
	}

	pitch = CalcF::lerp(pitch, target_pitch, dt * 50000.f);
	yaw = CalcF::lerp(yaw, target_yaw, dt * 50000.f);

	float corrected_pitch = pitch;
	float corrected_yaw = yaw + CalcF::PI/2.f;

	camera.set_forward(Vec3::spherical_to_cartesian(1.f, corrected_yaw, corrected_pitch));

	Vec3 basis[3] = {};
	basis[0] = Vec3::cross(camera.get_forward(), Vec3::up()).normalized();
	basis[1] = camera.get_forward();
	basis[2] = Vec3::cross(basis[0], basis[1]).normalized();

	for (int i = 0; i < 3; i++)
		basis[i] *= move_speed * dt;
	
	if (platform.kb_down[KEYBOARD_KEY_d])
		camera.move_by(basis[0]);

	if (platform.kb_down[KEYBOARD_KEY_a])
		camera.move_by(-basis[0]);

	if (platform.kb_down[KEYBOARD_KEY_w])
		camera.move_by(basis[1]);

	if (platform.kb_down[KEYBOARD_KEY_s])
		camera.move_by(-basis[1]);

	if (platform.kb_down[KEYBOARD_KEY_space])
		camera.move_by(basis[2]);

	if (platform.kb_down[KEYBOARD_KEY_left_shift] || platform.kb_down[KEYBOARD_KEY_right_shift])
		camera.move_by(-basis[2]);

	camera.recompute();

	platform.set_mouse_position(platform.window_width / 2, platform.window_height / 2);
}

void CameraDriver::toggle(bool enabled)
{
	active = enabled;
}

bool CameraDriver::is_active() const
{
	return active;
}

App::App(const Platform &platform)
	: platform(platform)
	, assets()
	, scratch_memory()
	, scratch_arenas()
	, global_timer(platform)
	, delta_timer(platform)
	, delta_accumulator()
	, graphics_device()
	, swapchain()
	, render_scene()
	, render_graph()
	, skybox_renderer()
//	, post_processing()
{
}

App::~App()
{
}

void App::init()
{
	scratch_memory = malloc(SCRATCH_MEMORY_SIZE * array_size(scratch_arenas));
	MemoryArena arena(scratch_memory, SCRATCH_MEMORY_SIZE * array_size(scratch_arenas));
	scratch_arenas[0] = arena.sub_arena(SCRATCH_MEMORY_SIZE);
	scratch_arenas[1] = arena.sub_arena(SCRATCH_MEMORY_SIZE);
	ScratchArena::select(scratch_arenas, array_size(scratch_arenas));

	graphics_device.init(platform);
	swapchain = graphics_device.create_swapchain(platform);
	render_graph.init(&graphics_device);

	assets.init(&platform, &graphics_device);

	render_scene.init(&graphics_device);

	ast::AssetHandle model_handle = assets.from_file_path("DamagedHelmet/DamagedHelmet.gltf", ast::ASSET_TYPE_MODEL);
	gfx::Model &model = assets.get_asset<ast::ModelAsset>(model_handle)->model;

	u32 object_handle = render_scene.create_object(Mat4::identity());
	gfx::RenderSceneObject &object = render_scene.get_object_from_handle(object_handle);
	object.mesh_handle = render_scene.upload_mesh(model.get_sub_model(0).mesh);
	object.material_handle = render_scene.upload_material(model.get_sub_model(0).material);

	render_scene.build_batches();
//	render_scene.merge_meshes();

	gfx::AttachmentInfo swapchain_attachment_info(VK_FORMAT_R32G32B32A32_SFLOAT);
	swapchain_attachment_info.size_class = gfx::AttachmentInfo::SIZE_CLASS_SWAPCHAIN_RELATIVE;
	swapchain_attachment_info.size_x = 1.f;
	swapchain_attachment_info.size_y = 1.f;
	swapchain_src = render_graph.create_texture_resource(swapchain_attachment_info);

	skybox_renderer.init(&graphics_device, assets, render_graph);
//	post_processing.start(&graphics_device, assets, render_graph);

	camera = gfx::Camera::perspective(Vec3::zero(), Vec3::forward(), 70.f, (float)DEFAULT_WINDOW_WIDTH / (float)DEFAULT_WINDOW_HEIGHT, 0.1f, 10.f);

	job_system.start(std::thread::hardware_concurrency());

	// Test out the job system.
	{
		job::SpinScope spin_scope(job_system);

		job_system.parallel_for(100, [&](int index) -> void {
			printf("(%d) Hello, World! From: %d\n", index, job::JobSystem::get_current_worker_id());
		}, job::PRIORITY_LOW, 1);
	}

	global_timer.start();
	delta_timer.start();
}

void App::destroy()
{
	job_system.shutdown();

	skybox_renderer.destroy();
//	post_processing.destroy();

	render_scene.destroy();

	assets.destroy();

	render_graph.destroy();

	graphics_device.destroy_swapchain(swapchain);
	graphics_device.destroy(platform);

	free(scratch_memory);
}

bool App::tick()
{
	if (platform.kb_pressed[KEYBOARD_KEY_escape]) {
		debug_log("Quitting...");
		return true;
	}

	const float elapsed_time = global_timer.get_elapsed_seconds();
	const float dt = delta_timer.reset();
	const float fixed_dt = 1.f / (float)platform.target_fps;

	update(dt);

	delta_accumulator += CalcF::min(dt, fixed_dt);

	while (delta_accumulator >= fixed_dt) {
		fixed_update(fixed_dt);
		delta_accumulator -= fixed_dt;
	}

	gfx::CommandBuffer cmd = graphics_device.begin_frame(swapchain);
	render(dt, elapsed_time, cmd);
	render_graph.set_swapchain_source(swapchain_src);
	render_graph.add_stage(gfx::RenderStage::TYPE_PRESENT);
	render_graph.execute(cmd, swapchain, dt, elapsed_time);
	graphics_device.end_frame(swapchain, cmd);

	return false;
}

void App::update(float dt)
{
	if (platform.kb_pressed[KEYBOARD_KEY_tab])
		camera_driver.toggle(!camera_driver.is_active());

	camera_driver.update(camera, platform, dt);

	render_scene.resolve_removing();
	render_scene.update();
}

void App::fixed_update(float dt)
{
}

void App::render(float dt, float elapsed_time, gfx::CommandBuffer &present_cmd)
{
	gfx::RenderGraphBlackboard bb;

	gfx::SceneView view = {};
	view.scene = &render_scene;
	view.camera = &this->camera;

	skybox_renderer.add_render_stages(render_graph, bb, view);

	gfx::SubresourceAlias alias = {};
	alias.parent = swapchain_src;
	alias.base_layer = 0;
	alias.base_mip = 0;
	alias.view_type = VK_IMAGE_VIEW_TYPE_2D;
	
	render_graph.move_subresource(skybox_renderer.output_attachment, alias);

//	post_processing.add_render_stages(render_graph, bb, view);

/*
	RenderResource cubemap = render_graph.create_texture_resource(...);

	gfx::MoveSubresourceDecl move_sub;
	move_sub.src = &output;
	move_sub.dst = &cubemap;
	move_sub.layer = 1;

	render_graph.move_subresource(move_sub);

	environment_probe_renderer.add_render_stages(render_graph, bb, view, cubemap);
*/
}
