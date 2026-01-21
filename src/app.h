#pragma once

#include "core/memory_arena.h"

#include "math/timer.h"

#include "assets/assets.h"

#include "graphics/device.h"
#include "graphics/render_scene.h"
#include "graphics/render_graph.h"
#include "graphics/camera.h"
#include "graphics/renderers/skybox_renderer.h"
#include "graphics/renderers/post_processing.h"

#include "job/job.h"

struct Platform;

class CameraDriver {
public:
	void update(gfx::Camera &camera, const Platform &platform, const inp::InputState &input, float dt);

	void toggle(bool enabled);
	bool is_active() const;

private:
	bool active = false;
	float yaw = 0.f;
	float target_yaw = 0.f;
	float pitch = 0.f;
	float target_pitch = 0.f;
};

class App {
public:
	App(const Platform &platform);
	~App();

	void init();
	void destroy();
	bool tick(const inp::InputState &input);

private:
	void update(float dt, const inp::InputState &input);
	void fixed_update(float dt);
	void render(float dt, float elapsed_time, gfx::CommandBuffer &present_cmd);

	const Platform &platform;

	ast::AssetManager assets;

	void *scratch_memory;
	MemoryArena scratch_arenas[2];

	Timer global_timer;
	Timer delta_timer;
	float delta_accumulator;

	gfx::Device graphics_device;
	gfx::Swapchain swapchain;

	gfx::RenderScene render_scene;
	gfx::RenderGraph render_graph;

	gfx::Camera camera;
	CameraDriver camera_driver;

	gfx::SkyboxRenderer skybox_renderer;
//	gfx::PostProcessingRenderer post_processing;

	gfx::RenderResourceHandle swapchain_src;
};
