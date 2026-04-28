#ifndef RENDER_PASS_CULLING_H
#define RENDER_PASS_CULLING_H

typedef struct R_CullFrustumClearPassData R_CullFrustumClearPassData;
struct R_CullFrustumClearPassData
{
	R_GraphBufHandle counter_handle;
};

R_PASS_RECORD_DEF(R_CullFrustumClearFn);


typedef struct R_CullFrustumPassData R_CullFrustumPassData;
struct R_CullFrustumPassData
{
	GFX_ShaderKey shader;
	
	R_GraphBufHandle indirect_handle;
	R_GraphBufHandle counter_handle;

	u64 object_buffer_address;
	u64 mesh_buffer_address;
	u64 page_table_buffer_address;
	
	u32 object_count;

	v4 frustum_planes[6];
};

R_PASS_RECORD_DEF(R_CullFrustumComputeFn);


typedef struct R_CullSphereClearPassData R_CullSphereClearPassData;
struct R_CullSphereClearPassData
{
	R_GraphBufHandle counter_handle;
};

R_PASS_RECORD_DEF(R_CullSphereClearFn);


typedef struct R_CullSpherePassData R_CullSpherePassData;
struct R_CullSpherePassData
{
	GFX_ShaderKey shader;
	
	R_GraphBufHandle indirect_handle;
	R_GraphBufHandle counter_handle;

	u64 object_buffer_address;
	u64 mesh_buffer_address;
	u64 page_table_buffer_address;
	
	u32 object_count;

	v4 sphere;
};

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
	AST_Assets *assets;
	AST_Handle frustum_shader;
	AST_Handle sphere_shader;
};

internal void R_CullingInit         (R_Culling *cull, AST_Assets *assets);
internal void R_CullingDestroy      (R_Culling *cull);

internal R_DrawStream R_CullFrustum (R_Culling *cull,
									 R_Graph *graph,
									 Arena *pass_arena,
									 const R_Scene *scene,
									 const R_SceneResources *scene_resources,
									 const R_FrustumVolume *frustum);

internal R_DrawStream R_CullSphere  (R_Culling *cull,
									 R_Graph *graph,
									 Arena *pass_arena,
									 const R_Scene *scene,
									 const R_SceneResources *scene_resources,
									 v3 sphere_centre, f32 sphere_radius);

#endif // RENDER_PASS_CULLING_H
