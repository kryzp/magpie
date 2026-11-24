#ifndef RENDER_PASS_CULLING_H
#define RENDER_PASS_CULLING_H

// todo: right now the stream output of the frustum vs shadow culling is of a fundamentally different packing / format
//       so this needs to be standardized/consolidated between them.
//       justification: it's pissing me off.

#include "core/types.h"
#include "graphics/resource_key.h"
#include "render/camera.h"
#include "render/frame.h"
#include "render/graph/pass.h"

namespace rnd {

class Graph;

// has matching SLANG values don't fuck with the values here
enum CullFilter : unsigned {
	CullFilter_OpaqueOnly = 0,
	CullFilter_AlphaOnly = 1,
	CullFilter_All = 2,
	CullFilter_MaxEnum
};

struct CullClearPassData {
	GraphBufHandle counter_handle;
};

void CullClearFn(const PassContext &ctx);

struct CullPassData {
	gfx::ResourceKey shader;

	GraphBufHandle indirect_handle;
	GraphBufHandle counter_handle;

	const FrameParams *frame_params = nullptr;

	CullFilter filter = CullFilter_OpaqueOnly;

	// frustum culling uses the planes, sphere culling uses the sphere.
	Vec4 frustum_planes[6];
	Vec4 sphere;
};

struct DrawStream {
	GraphBufHandle indirect_buffer;
	GraphBufHandle count_buffer;
};

// when generating the shadow render streams we create two streams each for opaque
// and alpha marked ones which get sent thru seperate pipelines/
// has matching SLANG values don't fuck with the values here
enum ShadowStream : unsigned {
	ShadowStream_Opaque = 0,
	ShadowStream_Masked = 1,
	ShadowStream_Count
};

inline unsigned ShadowStreamIndex(unsigned caster_index, ShadowStream stream)
{
	return caster_index * ShadowStream_Count + stream;
}

struct ShadowCullPassData {
	gfx::ResourceKey shader;

	GraphBufHandle indirect_handle;
	GraphBufHandle counter_handle;

	const FrameParams *frame_params = nullptr;

	unsigned caster_count = 0;
	u32 caster_mask = 0; // bit i set = cull for caster i this frame
};

DrawStream CullFrustum(
	Graph *graph,
	const FrameParams *frame_params,
	CullFilter filter,
	const FrustumVolume &frustum
);

// returned draw stream packs both opaque and masked shadow streams for all casters
// that need re-rendering.
// RecordIndirectStream(..., ShadowStreamIndex(c, s))
DrawStream CullShadowCasters(
	Graph *graph,
	const FrameParams *frame_params,
	unsigned caster_count,
	u32 caster_mask
);

}

#endif // RENDER_PASS_CULLING_H
