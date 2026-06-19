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

R_PASS_RECORD_DEF(R_CullClearFn);


typedef struct R_CullPassData R_CullPassData;
struct R_CullPassData
{
	G_ShaderKey shader;
	
	R_GraphBufHandle indirect_handle;
	R_GraphBufHandle counter_handle;

	u64 object_buffer_address;
	u64 page_table_buffer_address;
	
	R_CullFilter filter;

	union
	{
		v4 frustum_planes[6];
		v4 sphere;
	};
};

R_PASS_RECORD_DEF(R_CullFrustumComputeFn);
R_PASS_RECORD_DEF(R_CullSphereComputeFn);


typedef struct R_DrawStream R_DrawStream;
struct R_DrawStream
{
	R_GraphBufHandle indirect_buffer;
	R_GraphBufHandle count_buffer;
};

typedef struct R_Culling R_Culling;
struct R_Culling
{
	A_Registry *assets;
	
	A_Handle frustum_shader;
	A_Handle sphere_shader;
};

internal void R_CullingInit         (R_Culling *cull, A_Registry *assets);
internal void R_CullingDestroy      (R_Culling *cull);

internal R_DrawStream R_CullFrustum (R_Culling *cull,
									 R_Graph *graph,
									 const R_Bulletin *bt,
									 R_CullFilter filter,
									 const R_FrustumVolume *frustum);

internal R_DrawStream R_CullSphere  (R_Culling *cull,
									 R_Graph *graph,
									 const R_Bulletin *bt,
									 R_CullFilter filter,
									 v3 sphere_centre, f32 sphere_radius);

#endif // RENDER_PASS_CULLING_H
