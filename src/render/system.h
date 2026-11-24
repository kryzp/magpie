#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/types.h"
#include "core/log.h"
#include "graphics/resource_key.h"
#include "graphics/ring_buffer.h"
#include "graphics/swapchain.h"
#include "asset/handle.h"
#include "render/mesh.h"
#include "render/frame.h"
#include "render/pass/shadow_pass.h"
#include "render/debug/debug_renderer.h"

#define RENDER_SYSTEM_PREFILTER_MIPS 4

namespace rnd {

class Graph;

class System {
public:
	explicit System(LogChannel log_channel);
	~System();

	System(const System &) = delete;
	System &operator=(const System &) = delete;

	static System *Get();
	static void Select(System *system);

	gfx::RingBuffer *BeginFrameUploads(unsigned frame_slot_index);

	void BakeLookupsAndMaps(Graph *graph, const gfx::Swapchain *swapchain);
	void GenerateLookupsAndMaps(Graph *graph, const FrameParams *frame_params);
	void Render(Graph *graph, const FrameParams *frame_params);

	LogChannel GetLogChannel() const
	{
		return log_channel;
	}

	void EnableManualSRGB(bool enabled)
	{
		encode_rgb_output = enabled;
	}

	LogChannel log_channel;

	gfx::RingBuffer upload_ring;
	gfx::RingBuffer upload_slices[GRAPHICS_FRAMES_IN_FLIGHT];

	bool encode_rgb_output = false;
	float exposure = 0.2f;

	ast::Handle environment_hdr_handle;

	ShadowRenderer shadow_renderer;
	DebugRenderer debug_renderer;

	gfx::ResourceKey brdf_lut;
	gfx::ResourceKey environment_cubemap;
	gfx::ResourceKey irradiance_cubemap;
	gfx::ResourceKey prefilter_cubemap;

	Mesh skybox_mesh;

	gfx::ResourceKey cubemap_capture_transform_buffer;

	struct {
		ast::Handle debug_line_handle;
		ast::Handle depth_prepass_handle;
		ast::Handle forward_handle;
		ast::Handle shadow_handle;
		ast::Handle shadow_masked_handle;
		ast::Handle shadow_cull_handle;
		ast::Handle cull_frustum_handle;
		ast::Handle skybox_handle;
		ast::Handle tonemapping_handle;
		ast::Handle brdf_lut_generation_handle;
		ast::Handle hdr_to_cubemap_handle;
		ast::Handle irradiance_cubemap_gen_handle;
		ast::Handle prefilter_cubemap_gen_handle;
	} shaders;

	struct {
		gfx::ResourceKey linear;
		gfx::ResourceKey nearest;
		gfx::ResourceKey material;
		gfx::ResourceKey shadow_compare; // SampleCmp for shadow maps
	} samplers;
};

}

#endif // RENDER_SYSTEM_H
