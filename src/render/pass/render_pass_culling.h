#ifndef RENDER_PASS_CULLING_H
#define RENDER_PASS_CULLING_H

// has matching SLANG values don't fuck with
// the values here
typedef enum R_CullFilter R_CullFilter;
enum R_CullFilter
{
	R_CullFilter_OpaqueOnly = 0,
	R_CullFilter_AlphaOnly  = 1,
	R_CullFilter_All        = 2,
	R_CullFilter_COUNT
};

typedef struct R_CullClearPassData R_CullClearPassData;
struct R_CullClearPassData
{
	R_GraphBufHandle counter_handle;
};

static R_PASS_RECORD_DEF(R_CullClearFn);

typedef struct R_CullPassData R_CullPassData;
struct R_CullPassData
{
	G_ShaderKey shader;
	
	R_GraphBufHandle indirect_handle;
	R_GraphBufHandle counter_handle;
	
	const R_FrameParams *frame_params;

	R_CullFilter filter;

	union
	{
		v4 frustum_planes[6];
		v4 sphere;
	};
};

static R_PASS_RECORD_DEF(R_CullFrustumComputeFn);
static R_PASS_RECORD_DEF(R_CullSphereComputeFn);

typedef struct R_DrawStream R_DrawStream;
struct R_DrawStream
{
	R_GraphBufHandle indirect_buffer;
	R_GraphBufHandle count_buffer;
};

static R_DrawStream R_CullFrustum(R_Graph *graph,
								  const R_FrameParams *frame_params,
								  R_CullFilter filter,
								  const R_FrustumVolume *frustum);

static R_DrawStream R_CullSphere(R_Graph *graph,
								 const R_FrameParams *frame_params,
								 R_CullFilter filter,
								 v3 sphere_centre, f32 sphere_radius);

#endif // RENDER_PASS_CULLING_H
