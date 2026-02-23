#pragma once

#include "core/memory_arena.h"

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

private:
	float yaw = 0.f;
	float target_yaw = 0.f;
	float pitch = 0.f;
	float target_pitch = 0.f;
};

class App {
	constexpr static u32 TARGET_FPS = 120;

	static constexpr u64 TRANSIENT_ARENA_SIZE = MEGABYTES(512);

public:
	App();
	~App();

	void init(VirtualArena &global_arena);
	void destroy();
	bool tick(const inp::InputState &input);

private:
	void update(float dt, const inp::InputState &input);
	void fixed_update(float dt);
	void render(float dt, const inp::InputState &input, float elapsed_time, gfx::CommandBuffer &present_cmd, gfx::RenderSceneResources &scene_resources);

	void add_imgui_render_stage(gfx::RenderGraph &graph, const gfx::RenderResourceHandle &output_attachment);
	
	Timer global_timer;
	Timer delta_timer;
	float delta_accumulator;

	ast::AssetManager assets;

	gfx::Device graphics_device;
	gfx::Swapchain swapchain;

	gfx::RenderScene render_scene;
	gfx::RenderGraph render_graph;

	gfx::GpuRingBuffer ring_upload_buffer;

	gfx::Camera camera;
	CameraDriver camera_driver;
	bool camera_driver_active;

	gfx::GpuBuffer *frame_data_buffer;
	gfx::GpuBuffer *cubemap_capture_transforms;
	
	gfx::Texture *brdf_texture;

	gfx::Texture *irradiance_cubemap;
	gfx::Texture *prefilter_cubemap;

	gfx::IBLRenderer ibl_renderer;
	gfx::ComputeCulling compute_culling;
	gfx::DeferredRenderer deferred_renderer;
	gfx::SkyboxRenderer skybox_renderer;
	gfx::PostProcessingRenderer post_processing;

	gfx::RenderResourceHandle swapchain_src;

	gfx::RenderHandle light_handle;
};
