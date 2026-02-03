#include "app.h"

#include "ext/imgui/imgui.h"

#include "core/scratch.h"
#include "platform/platform.h"
#include "math/calc.h"
#include "assets/model_serializer.h"
#include "assets/texture_serializer.h"
#include "graphics/gpu_types.h"

void CameraDriver::update(gfx::Camera &camera, const inp::InputState &input, float dt)
{
	const float mouse_deadzone = .001f;
	const float turn_speed = 0.1f;
	const float move_speed = 2.5f;

	const float dx = input.mouse_delta.x;
	const float dy = input.mouse_delta.y;

	if (dx*dx + dy*dy > mouse_deadzone*mouse_deadzone) {
		target_yaw -= dx * turn_speed;
		target_pitch -= dy * turn_speed;
	}

	pitch = CalcF::lerp(pitch, target_pitch, 35.f * dt);
	yaw = CalcF::lerp(yaw, target_yaw, 35.f * dt);

	float corrected_pitch = pitch;
	float corrected_yaw = yaw + CalcF::PI*0.5f;

	camera.set_forward(Vec3::spherical_to_cartesian(1.f, corrected_yaw * CalcF::DEG2RAD, corrected_pitch * CalcF::DEG2RAD));

	Vec3 basis[3] = {};
	basis[0] = Vec3::cross(camera.get_forward(), Vec3::up()).normalized();
	basis[1] = camera.get_forward();
	basis[2] = Vec3::cross(basis[0], basis[1]).normalized();

	for (int i = 0; i < array_size(basis); i++)
		basis[i] *= move_speed * dt;

	float hori = input.kb_down[inp::KEYBOARD_KEY_d]	    -  input.kb_down[inp::KEYBOARD_KEY_a];
	float frwd = input.kb_down[inp::KEYBOARD_KEY_w]	    -  input.kb_down[inp::KEYBOARD_KEY_s];
	float vert = input.kb_down[inp::KEYBOARD_KEY_space] - (input.kb_down[inp::KEYBOARD_KEY_left_shift] + input.kb_down[inp::KEYBOARD_KEY_right_shift]);

	camera.move_by(basis[0] * hori);
	camera.move_by(basis[1] * frwd);
	camera.move_by(basis[2] * vert);

	camera.recompute();
}

App::App()
	: scratch_memory(nullptr)
	, scratch_arenas{}
	, class_db()
	, global_timer()
	, delta_timer()
	, delta_accumulator()
	, assets()
	, graphics_device()
	, swapchain()
	, render_scene()
	, render_graph()
	, camera()
	, camera_driver()
	, camera_driver_active(false)
	, frame_data_buffer(nullptr)
	, cubemap_capture_transforms(nullptr)
	, brdf_texture(nullptr)
	, irradiance_cubemap(nullptr)
	, prefilter_cubemap(nullptr)
	, ibl_renderer()
	, compute_culling()
	, deferred_renderer()
	, skybox_renderer()
	, post_processing()
	, swapchain_src()
	, light_handle()
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

	graphics_device.init();
	swapchain = graphics_device.create_swapchain();

	graphics_device.init_imgui();
	
	gfx::Sampler::linear = graphics_device.create_sampler(VK_FILTER_LINEAR);

	assets.init(&graphics_device);
	
	render_graph.init(&graphics_device);
	
	render_scene.init(&graphics_device);

	ast::AssetHandle model_handle = assets.from_file_path("Models/Sponza/glTF/Sponza.gltf");
//	ast::AssetHandle model_handle = assets.from_file_path("Models/DamagedHelmet/glTF/DamagedHelmet.gltf");

	gfx::Model &model = assets.get_asset<ast::ModelAsset>(model_handle)->model;

	assets.wait_for_async_uploads();

	assets.flush_uploads();

	for (auto &sub : model.sub_models) {
		Mat4 transform = sub.transform;
		u32 mesh = render_scene.register_mesh(sub.mesh);
		u32 material = render_scene.register_material(sub.material, assets);

		render_scene.create_object(transform, mesh, material);
	}

	light_handle = render_scene.create_light({
		.type = gfx::Light::TYPE_POINT,
		.position = Vec3(0.f, 0.f, 1.f),
		.colour = { 255, 255, 255, 255 },
		.intensity = 0.f,
		.falloff = 1.f
	});

	frame_data_buffer = graphics_device.alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(gfx::gpu_types::GpuFrameData)
	);

	Mat4 capture_view_matrices[] = {
		Mat4::lookat(Vec3::zero(), Vec3( 1.f,  0.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Right
		Mat4::lookat(Vec3::zero(), Vec3(-1.f,  0.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Left
		Mat4::lookat(Vec3::zero(), Vec3( 0.f,  0.f,  1.f), Vec3(0.f, -1.f, 0.f)), // Up
		Mat4::lookat(Vec3::zero(), Vec3( 0.f,  0.f, -1.f), Vec3(0.f,  1.f, 0.f)), // Down
		Mat4::lookat(Vec3::zero(), Vec3( 0.f,  1.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Forward
		Mat4::lookat(Vec3::zero(), Vec3( 0.f, -1.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Backwards
	};

	Mat4 capture_projection_matrix = Mat4::perspective(90.f, 1.f, 0.1f, 10.f);

	for (int i = 0; i < 6; i++)
		capture_view_matrices[i] = capture_projection_matrix * capture_view_matrices[i];

	cubemap_capture_transforms = graphics_device.alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(capture_view_matrices)
	);

	cubemap_capture_transforms->write(capture_view_matrices, sizeof(capture_view_matrices), 0);

	ibl_renderer.init(assets);
	compute_culling.init(assets);
	deferred_renderer.init(&graphics_device, assets);
	skybox_renderer.init(&graphics_device, assets);
	post_processing.init(assets);
	
	camera = gfx::Camera::perspective(Vec3::zero(), Vec3::forward(), 90.f, (float)DEFAULT_WINDOW_WIDTH / (float)DEFAULT_WINDOW_HEIGHT, 0.1f, 100.f);

	global_timer.start();
	delta_timer.start();

	brdf_texture = graphics_device.alloc_texture_2d(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);
	irradiance_cubemap = graphics_device.alloc_texture_cubemap(32, VK_FORMAT_R32G32B32A32_SFLOAT, 1);
	prefilter_cubemap = graphics_device.alloc_texture_cubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, 5);
	
	const gfx::Texture *hdr_texture = assets.get_asset<ast::TextureAsset>(assets.from_file_path("environment_map_1.hdr"))->texture;

	skybox_renderer.render_hdr_to_skybox(
		render_graph,
		hdr_texture,
		cubemap_capture_transforms
	);

	ibl_renderer.render_brdf(
		render_graph,
		brdf_texture
	);
	
	ibl_renderer.render_environment_map(
		render_graph,
		irradiance_cubemap,
		prefilter_cubemap,
		skybox_renderer.get_environment_map(),
		skybox_renderer.get_mesh(),
		cubemap_capture_transforms
	);
}

void App::destroy()
{
	graphics_device.graphics().wait_idle();

	graphics_device.destroy_texture(brdf_texture);
	graphics_device.destroy_texture(irradiance_cubemap);
	graphics_device.destroy_texture(prefilter_cubemap);
	
	graphics_device.destroy_buffer(cubemap_capture_transforms);

	graphics_device.destroy_sampler(gfx::Sampler::linear);

	ibl_renderer.destroy();
	compute_culling.destroy();
	deferred_renderer.destroy();
	skybox_renderer.destroy();
	post_processing.destroy();

	render_scene.destroy();

	graphics_device.destroy_buffer(frame_data_buffer);

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
	
	assets.flush_uploads();
	
	update(dt, input);

	delta_accumulator += CalcF::min(dt, fixed_dt);

	while (delta_accumulator >= fixed_dt) {
		fixed_update(fixed_dt);
		delta_accumulator -= fixed_dt;
	}

	ImGui::Begin("Params");
	{
		static float exp = 1.2f;

		if (ImGui::SliderFloat("Exposure", &exp, 0.f, 2.5f))
			post_processing.set_exposure(exp);

		static float intens = 0.0f;

		if (ImGui::SliderFloat("Light Intensity", &intens, 0.f, 5.f))
			render_scene.set_light_intensity(light_handle, intens);
	}
	ImGui::End();

	ImGui::Begin("Info");
	{
		static float fps_samples[512] = {};
		static unsigned current_sample = 0;

		fps_samples[current_sample % array_size(fps_samples)] = 1.f / dt;

		float fps_average = 0.f;

		for (int i = 0; i < array_size(fps_samples); i++)
			fps_average += fps_samples[i];

		fps_average /= (float)array_size(fps_samples);

		current_sample++;

		ImGui::Text("FPS Average: %d", (int)fps_average);
		ImGui::Text("Alpha: %f", delta_accumulator / fixed_dt);
		ImGui::Text("Time: %f", elapsed_time);
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
			global_timer.reset();
	}
	ImGui::End();

	gfx::CommandBuffer cmd = graphics_device.begin_frame(swapchain);
	{
		gfx::RenderSceneResources scene_resources = render_scene.update_transient_resources(render_graph);

		render(dt, input, elapsed_time, cmd, scene_resources);
		add_imgui_render_stage(render_graph, swapchain_src);

		render_graph.set_backbuffer_source(swapchain_src);

		render_graph.compile(swapchain);
		render_graph.execute(cmd, swapchain, render_scene, camera, dt, elapsed_time);
		render_graph.reset();
	}
	graphics_device.end_frame(swapchain, cmd);

	return false;
}

void App::update(float dt, const inp::InputState &input)
{
	if (input.kb_pressed[inp::KEYBOARD_KEY_tab]) {
		camera_driver_active = !camera_driver_active;
		platform::set_mouse_locked(camera_driver_active);
	}

	if (camera_driver_active) {
		camera_driver.update(camera, input, dt);

		int window_width, window_height;
		platform::get_window_size(&window_width, &window_height);

		platform::set_mouse_position(window_width / 2.f, window_height / 2.f);
	}

	if (input.gamepads[0].pressed[inp::GAMEPAD_BUTTON_cross])
		inp::rumble_gamepad(0, input.gamepads[0].left_trigger, input.gamepads[0].right_trigger, 0.25f);
}

void App::fixed_update(float dt)
{
}

void App::render(float dt, const inp::InputState &input, float elapsed_time, gfx::CommandBuffer &present_cmd, gfx::RenderSceneResources &scene_resources)
{
	gfx::gpu_types::GpuFrameData data = {};
	data.view = camera.get_view();
	data.projection = camera.get_projection();
	data.view_projection = camera.get_projection() * camera.get_view();
	data.view_projection_no_translation = camera.get_projection() * camera.get_view().remove_translation();
	data.inv_view = data.view.inverse();
	data.inv_projection = data.projection.inverse();
	data.camera_position = camera.get_position();
	data.window_resolution = Vec2(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
	data.time = elapsed_time;

	frame_data_buffer->write(&data, sizeof(gfx::gpu_types::GpuFrameData), 0);

	struct FooBar { int baz; };

	render_graph.push_stage<FooBar>(
		"Swapchain Src",
		gfx::RenderStage::TYPE_TRANSFER,
		[&](gfx::RenderGraphBuilder &builder, FooBar &data) { swapchain_src = builder.create_texture(gfx::AttachmentInfo(VK_FORMAT_R32G32B32A32_SFLOAT)); },
		[=](const gfx::RenderContext &ctx, const gfx::RenderStageResources &resources, const FooBar &data) { }
	);
	
	gfx::RenderGraphBlackboard bb;

	compute_culling.add_render_stages(render_graph, bb, render_scene, scene_resources);
	deferred_renderer.add_render_stages(render_graph, bb, scene_resources, frame_data_buffer, render_graph.import_texture(irradiance_cubemap), render_graph.import_texture(prefilter_cubemap), render_graph.import_texture(brdf_texture));
	skybox_renderer.add_render_stages(render_graph, bb, frame_data_buffer);
	post_processing.add_render_stages(render_graph, bb, swapchain_src);
	
	auto &gbuffer = bb.get<gfx::DeferredRendererInfo>().gbuffer;

	     if (input.kb_down[inp::KEYBOARD_KEY_d1]) swapchain_src = gbuffer.attachments[gfx::GBuffer::ATTACHMENT_POSITION];
	else if (input.kb_down[inp::KEYBOARD_KEY_d2]) swapchain_src = gbuffer.attachments[gfx::GBuffer::ATTACHMENT_ALBEDO];
	else if (input.kb_down[inp::KEYBOARD_KEY_d3]) swapchain_src = gbuffer.attachments[gfx::GBuffer::ATTACHMENT_NORMAL];
	else if (input.kb_down[inp::KEYBOARD_KEY_d4]) swapchain_src = gbuffer.attachments[gfx::GBuffer::ATTACHMENT_EMISSIVE];
	else if (input.kb_down[inp::KEYBOARD_KEY_d5]) swapchain_src = gbuffer.attachments[gfx::GBuffer::ATTACHMENT_METALLIC_ROUGHNESS];
	else if (input.kb_down[inp::KEYBOARD_KEY_d6]) swapchain_src = gbuffer.lighting;
}

void App::add_imgui_render_stage(gfx::RenderGraph &graph, const gfx::RenderResourceHandle &output_attachment)
{
	struct ImGuiStageData {
		int foo;
	};

	graph.push_stage<ImGuiStageData>(
		"ImGui",
		gfx::RenderStage::TYPE_GRAPHICS,
		[&](gfx::RenderGraphBuilder &builder, ImGuiStageData &data) {
			builder.write_colour(output_attachment);
		},
		[=](const gfx::RenderContext &ctx, const gfx::RenderStageResources &resources, const ImGuiStageData &data) {
			ImGui::Render();
			ctx.device.imgui_record_draw_data(ctx.cmd);
		}
	);
}
