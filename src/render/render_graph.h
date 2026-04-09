#ifndef RENDER_GRAPH_H
#define RENDER_GRAPH_H

#define R_GRAPH_MAX_TEX_RESOURCES  256
#define R_GRAPH_MAX_BUF_RESOURCES  128
#define R_GRAPH_MAX_TEX_VERSIONS   512
#define R_GRAPH_MAX_BUF_VERSIONS   256
#define R_GRAPH_MAX_PASSES         64
#define R_GRAPH_MAX_IMPORTS        64


/* ==================================================
   VIRTUAL RESOURCES
   ================================================== */

typedef struct R_GraphTexture R_GraphTexture;
struct R_GraphTexture
{
	GFX_TextureKey physical_key;
	R_TextureInfo texture_info;
	
	u32 first_pass_index;
	u32 last_pass_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	GFX_TextureKey imported_key;
};

typedef struct R_GraphBuffer R_GraphBuffer;
struct R_GraphBuffer
{
	GFX_BufferKey physical_key;
	R_BufferInfo buffer_info;
	
	u32 first_pass_index;
	u32 last_pass_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	GFX_BufferKey imported_key;
};


/* ==================================================
   VERSIONING
   
   Every write produces a new version. Every read
   consumes an existing version.
   ================================================== */

typedef struct R_GraphTexVersion R_GraphTexVersion;
struct R_GraphTexVersion
{
	u32 resource_index;
	u32 writer_pass; // R_GRAPH_INVALID_INDEX if initial
	u32 parent;      // R_GRAPH_INVALID_INDEX if initial
};

typedef struct R_GraphBufVersion R_GraphBufVersion;
struct R_GraphBufVersion
{
	u32 resource_index;
	u32 writer_pass;
	u32 parent;
};


/* ==================================================
   IMPORTS
   ================================================== */

typedef struct R_GraphImportedTexture R_GraphImportedTexture;
struct R_GraphImportedTexture
{
	GFX_TextureKey external_key;
	R_GraphTexHandle handle;
};

typedef struct R_GraphImportedBuffer R_GraphImportedBuffer;
struct R_GraphImportedBuffer
{
	GFX_BufferKey external_key;
	R_GraphBufHandle handle;
};


/* ==================================================
   GRAPH
   ================================================== */

typedef struct R_Graph R_Graph;
struct R_Graph
{
	Arena *permanent_arena;
	Arena *frame_arena;

	u32 texture_res_count; R_GraphTexture    texture_res[R_GRAPH_MAX_TEX_RESOURCES];
	u32 texture_ver_count; R_GraphTexVersion texture_ver[R_GRAPH_MAX_TEX_VERSIONS];

	u32 buffer_res_count; R_GraphBuffer     buffer_res[R_GRAPH_MAX_BUF_RESOURCES];
	u32 buffer_ver_count; R_GraphBufVersion buffer_ver[R_GRAPH_MAX_BUF_VERSIONS];

	u32 imported_texture_count; R_GraphImportedTexture imported_textures[R_GRAPH_MAX_IMPORTS];
	u32 imported_buffer_count;  R_GraphImportedBuffer  imported_buffers[R_GRAPH_MAX_IMPORTS];
	
	u32 pass_count;
	R_Pass passes[R_GRAPH_MAX_PASSES];

	R_ResourcePool pool;
	R_ResourceTracker tracker;

	R_GraphTexHandle backbuffer_handle;
};


/* ==================================================
   CORE
   ================================================== */

internal void R_GraphInit(R_Graph *graph, Arena *permanent_arena, Arena *frame_arena);
internal void R_GraphDestroy(R_Graph *graph, GFX_Device *device);

internal void R_GraphReset(R_Graph *graph, const GFX_Device *device);


/* ==================================================
   BUILDING
   ================================================== */

// --- Passes

internal R_Pass *R_GraphAdd(R_Graph *graph, String8 name, R_PassType type);


// --- Resources

internal R_GraphTexHandle R_GraphCreateTexture (R_Graph *graph, const R_TextureInfo *info);
internal R_GraphBufHandle R_GraphCreateBuffer  (R_Graph *graph, const R_BufferInfo *info);

internal R_GraphTexHandle R_GraphImportTexture (R_Graph *graph, const GFX_Device *device, GFX_TextureKey external_key);
internal R_GraphBufHandle R_GraphImportBuffer  (R_Graph *graph, const GFX_Device *device, GFX_BufferKey external_key);


// --- Versioning

internal R_GraphTexHandle R_GraphPushTexVersion (R_Graph *graph, R_GraphTexHandle parent, u32 writer_pass_index);
internal R_GraphBufHandle R_GraphPushBufVersion (R_Graph *graph, R_GraphBufHandle parent, u32 writer_pass_index);


/* ==================================================
   COMPILATION
   ================================================== */

internal void R_GraphCompile(R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain);

internal void R_GraphPropagateDependencies     (R_Graph *graph);
internal void R_GraphBackpropagateDependencies (R_Graph *graph);
internal void R_GraphAllocateResources         (R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain);
internal void R_GraphGenerateBarriers          (R_Graph *graph, const GFX_Device *device);

internal void R_GraphProcessInvalidateTexture (R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassTextureEdge *edge);
internal void R_GraphProcessInvalidateBuffer  (R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassBufferEdge *edge);

internal void R_GraphProcessFlushTexture (R_Graph *graph, const R_PassTextureEdge *edge);
internal void R_GraphProcessFlushBuffer  (R_Graph *graph, const R_PassBufferEdge *edge);


/* ==================================================
   EXECUTION
   ================================================== */

internal void R_GraphExecute(R_Graph *graph,
							 GFX_Device *device,
							 const GFX_Swapchain *swapchain,
							 GFX_CmdBuffer *cmd,
							 const R_Scene *scene,
							 const R_Camera *camera,
							 f32 delta_time, f32 elapsed_time);

internal void R_GraphPresentToSwapchain(R_Graph *graph,
										const GFX_Device *device,
										const GFX_Swapchain *swapchain,
										GFX_CmdBuffer *cmd);


/* ==================================================
   RESOURCE RESOLUTION
   ================================================== */

// TODO: Should these return GFX_XxxKey's instead?

internal const GFX_Texture     *R_GraphResolveTexture     (const R_Graph *graph, const GFX_Device *device, R_GraphTexHandle handle);
internal const GFX_TextureView *R_GraphResolveTextureView (const R_Graph *graph,       GFX_Device *device, R_GraphTexHandle handle, GFX_SubresourceRange range);
internal const GFX_Buffer      *R_GraphResolveBuffer      (const R_Graph *graph, const GFX_Device *device, R_GraphBufHandle handle);
internal GFX_BufferRange        R_GraphResolveBufferRange (const R_Graph *graph, const GFX_Device *device, R_GraphBufHandle handle);


/* ==================================================
   HELPERS
   ================================================== */

internal void R_GraphSetBackbuffer(R_Graph *graph, R_GraphTexHandle handle);

internal GFX_RenderInfo R_GraphBuildRenderingInfo(const R_Graph *graph, GFX_Device *device, const R_Pass *pass);

internal R_GraphTexture *R_GraphTextureFromHandle (R_Graph *graph, R_GraphTexHandle handle);
internal R_GraphBuffer  *R_GraphBufferFromHandle  (R_Graph *graph, R_GraphBufHandle handle);

internal b32 R_GraphTexVersionIsUnwritten (const R_Graph *graph, R_GraphTexHandle handle);
internal b32 R_GraphBufVersionIsUnwritten (const R_Graph *graph, R_GraphBufHandle handle);

// Check if any requested access bits haven't
// been made visible yet in the stages that
// need them.
internal b32 R_ResourceNeedsInvalidation(const GFX_AccessSt *access, const R_ResourceState *state);


#endif // RENDER_GRAPH_H
