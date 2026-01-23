#include "app.h"

#include "ext/imgui/imgui.h"

#include "core/scratch.h"
#include "platform/platform.h"
#include "math/calc.h"
#include "assets/model_serializer.h"

void CameraDriver::update(gfx::Camera &camera, const inp::InputState &input, float dt)
{
	if (!active)
		return;
	
	const float mouse_deadzone = .001f;
	const float turn_speed = 0.7f;
	const float move_speed = 5000.f;

	int window_width, window_height;
	platform::get_window_size(&window_width, &window_height);

	float dx = (float)(input.mouse_position.x - window_width/2);
	float dy = (float)(input.mouse_position.y - window_height/2);

	if (dx*dx + dy*dy > mouse_deadzone*mouse_deadzone) {
		target_yaw -= dx * turn_speed * dt;
		target_pitch -= dy * turn_speed * dt;
	}

	pitch = CalcF::lerp(pitch, target_pitch, dt * 20.f);
	yaw = CalcF::lerp(yaw, target_yaw, dt * 20.f);

	float corrected_pitch = pitch;
	float corrected_yaw = yaw + CalcF::PI/2.f;

	camera.set_forward(Vec3::spherical_to_cartesian(1.f, corrected_yaw, corrected_pitch));

	Vec3 basis[3] = {};
	basis[0] = Vec3::cross(camera.get_forward(), Vec3::up()).normalized();
	basis[1] = camera.get_forward();
	basis[2] = Vec3::cross(basis[0], basis[1]).normalized();

	for (int i = 0; i < array_size(basis); i++)
		basis[i] *= move_speed * dt;
	
	if (input.kb_down[inp::KEYBOARD_KEY_d])
		camera.move_by(basis[0]);

	if (input.kb_down[inp::KEYBOARD_KEY_a])
		camera.move_by(-basis[0]);

	if (input.kb_down[inp::KEYBOARD_KEY_w])
		camera.move_by(basis[1]);

	if (input.kb_down[inp::KEYBOARD_KEY_s])
		camera.move_by(-basis[1]);

	if (input.kb_down[inp::KEYBOARD_KEY_space])
		camera.move_by(basis[2]);

	if (input.kb_down[inp::KEYBOARD_KEY_left_shift] || input.kb_down[inp::KEYBOARD_KEY_right_shift])
		camera.move_by(-basis[2]);

	camera.recompute();

	platform::set_mouse_position(window_width / 2, window_height / 2);
}

void CameraDriver::toggle(bool enabled)
{
	active = enabled;
}

bool CameraDriver::is_active() const
{
	return active;
}

App::App()
	: assets()
	, scratch_memory()
	, scratch_arenas()
	, global_timer()
	, delta_timer()
	, delta_accumulator()
	, graphics_device()
	, swapchain()
	, render_scene()
	, render_graph()
	, skybox_renderer()
	, post_processing()
{
}

App::~App()
{
}

static void my_parallel_for_test(u32 i)
{
	debug_log("(%d) From: %d", i, job::get_current_worker_id());
}

void App::init()
{
	scratch_memory = malloc(SCRATCH_MEMORY_SIZE * array_size(scratch_arenas));
	MemoryArena arena(scratch_memory, SCRATCH_MEMORY_SIZE * array_size(scratch_arenas));
	scratch_arenas[0] = arena.sub_arena(SCRATCH_MEMORY_SIZE);
	scratch_arenas[1] = arena.sub_arena(SCRATCH_MEMORY_SIZE);
	ScratchArena::select(scratch_arenas, array_size(scratch_arenas));

	graphics_device.init();
	swapchain = graphics_device.create_swapchain();

	graphics_device.init_imgui();

	render_graph.init(&graphics_device);

	assets.init(&graphics_device);

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
	post_processing.init(&graphics_device, assets, render_graph);
	
	camera = gfx::Camera::perspective(Vec3::zero(), Vec3::forward(), 70.f, (float)DEFAULT_WINDOW_WIDTH / (float)DEFAULT_WINDOW_HEIGHT, 0.1f, 10.f);

	// Test out the job system.
	{
		JOB_SPIN_SCOPE();
		job::parallel_for(100, my_parallel_for_test);
	}

	global_timer.start();
	delta_timer.start();
}

void App::destroy()
{
	skybox_renderer.destroy();
	post_processing.destroy();

	render_scene.destroy();

	assets.destroy();

	render_graph.destroy();

	graphics_device.destroy_swapchain(swapchain);
	graphics_device.destroy();

	free(scratch_memory);
}

bool App::tick(const inp::InputState &input)
{
	if (input.kb_pressed[inp::KEYBOARD_KEY_escape]) {
		debug_log("Quitting...");
		return true;
	}
	
	graphics_device.imgui_new_frame();
	ImGui::NewFrame();

	const float elapsed_time = global_timer.get_elapsed_seconds();
	const float dt = delta_timer.reset();
	const float fixed_dt = 1.f / (float)TARGET_FPS;
	
	update(dt, input);

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

void App::update(float dt, const inp::InputState &input)
{
	if (input.kb_pressed[inp::KEYBOARD_KEY_tab])
		camera_driver.toggle(!camera_driver.is_active());

	camera_driver.update(camera, input, dt);

	// Test out gamepad support.
	{
		if (input.gamepads[0].pressed[inp::GAMEPAD_BUTTON_cross])
			inp::rumble_gamepad(0, input.gamepads[0].left_trigger, input.gamepads[0].right_trigger, 0.25f);
	}

	ImGui::Begin("Params");
	{
		static float exp = 1.5f;

		if (ImGui::SliderFloat("Exposure", &exp, 0.f, 2.5f))
			post_processing.set_exposure(exp);
	}
	ImGui::End();

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
	post_processing.add_render_stages(render_graph, bb, view, skybox_renderer.output_attachment);
	
	gfx::RenderStage &stage = render_graph.add_stage(gfx::RenderStage::TYPE_GRAPHICS);

	stage.add_colour_output(post_processing.output_attachment);

	stage.set_record([&](const gfx::RenderContext &ctx) -> void {
		gfx::CommandBuffer &cmd = ctx.cmd;
		ImGui::Render();
		graphics_device.imgui_record_draw_data(cmd);
	});

	gfx::SubresourceAlias alias = {};
	alias.parent = swapchain_src;
	alias.base_layer = 0;
	alias.base_mip = 0;
	alias.view_type = VK_IMAGE_VIEW_TYPE_2D;
	
	render_graph.move_subresource(post_processing.output_attachment, alias);
}
