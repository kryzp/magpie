#ifndef RENDER_PASS_SHADOW_H
#define RENDER_PASS_SHADOW_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "graphics/ring_buffer.h"
#include "render/frame.h"
#include "render/blackboard.h"
#include "render/pass/culling_pass.h"

#define RENDER_SHADOW_MAP_RESOLUTION 1024

#define RENDER_SHADOW_MAP_FORMAT VK_FORMAT_D32_SFLOAT

// we use reversed-z so the receiver is lit when it's depth is >= the stored occluder depth.
#define RENDER_SHADOW_COMPARE_OP VK_COMPARE_OP_GREATER_OR_EQUAL

// we also apply a normal offset SHADOW_NORMAL_OFFSET_TEXELS later
// todo: is this necessary?
#define RENDER_SHADOW_DEPTH_BIAS_CONSTANT         -2.f
#define RENDER_SHADOW_DEPTH_BIAS_SLOPE            -2.f

namespace rnd {

class Graph;

struct ShadowMappingPassData {
	const FrameParams *frame_params = nullptr;
	unsigned caster_index = 0;
	DrawStream draw_stream; // must come from CullShadowCasters!!!
};

void ShadowMappingPassFn(const PassContext &ctx);

class ShadowRenderer {
public:
	ShadowRenderer();
	~ShadowRenderer();

	ShadowRenderer(const ShadowRenderer &) = delete;
	ShadowRenderer &operator=(const ShadowRenderer &) = delete;

	// (also recomputes all caster's content_hash)
	void BuildCasterTable(gfx::RingBuffer *upload, FrameParams *frame_params) const;

	void Render(Graph *graph, const FrameParams *frame_params, Blackboard *bb);

	void InvalidateCache();

private:
	gfx::ResourceKey shadow_cubemaps[RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS];
	gfx::ResourceKey shadow_cubemap_views[RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS];

	bool cache_valid[RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS] = {};
	u64 cached_hash[RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS] = {};
};

}

#endif // RENDER_PASS_SHADOW_H
