#pragma once

#include "core/memory_arena.h"
#include "core/class_db.h"

#include "math/timer.h"

#include "assets/assets.h"

#include "graphics/device.h"
#include "graphics/render_scene.h"
#include "graphics/render_graph.h"
#include "graphics/camera.h"

#include "graphics/renderers/ibl_renderer.h"
#include "graphics/renderers/compute_culling.h"
#include "graphics/renderers/deferred_renderer.h"
#include "graphics/renderers/skybox_renderer.h"
#include "graphics/renderers/post_processing.h"

#include "platform/input.h"

class CameraDriver {
public:
	void update(gfx::Camera &camera, const inp::InputState &input, float dt);

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
	constexpr static u32 TARGET_FPS = 120;

public:
	App();
	~App();

	void init();
	void destroy();
	bool tick(const inp::InputState &input);

private:
	void update(float dt, const inp::InputState &input);
	void fixed_update(float dt);
	void render(float dt, float elapsed_time, gfx::CommandBuffer &present_cmd);

	void add_imgui_render_stage(gfx::RenderGraph &graph, const gfx::RenderResourceHandle &output_attachment);

	ast::AssetManager assets;

	void *scratch_memory;
	MemoryArena scratch_arenas[2];

	ClassDB class_db;

	Timer global_timer;
	Timer delta_timer;
	float delta_accumulator;

	gfx::Device graphics_device;
	gfx::Swapchain swapchain;

	gfx::RenderScene render_scene;
	gfx::RenderGraph render_graph;

	gfx::Camera camera;
	CameraDriver camera_driver;

	gfx::GpuBuffer *frame_data_buffer;

	gfx::GpuBuffer *cubemap_capture_transforms;

	gfx::Texture *irradiance_cubemap;
	gfx::Texture *prefilter_cubemap;
	
	gfx::Texture *brdf_texture;

	gfx::IBLRenderer ibl_renderer;
	gfx::ComputeCulling compute_culling;
	gfx::DeferredRenderer deferred_renderer;
	gfx::SkyboxRenderer skybox_renderer;
	gfx::PostProcessingRenderer post_processing;

	gfx::RenderResourceHandle swapchain_src;
};
