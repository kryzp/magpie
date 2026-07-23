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
	G_ResourceKey physical_key;
	R_TextureInfo texture_info;
	
	u32 first_pass_index;
	u32 last_pass_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	G_ResourceKey imported_key;
};

typedef struct R_GraphBuffer R_GraphBuffer;
struct R_GraphBuffer
{
	G_ResourceKey physical_key;
	R_BufferInfo buffer_info;
	
	u32 first_pass_index;
	u32 last_pass_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	G_ResourceKey imported_key;
};


/* ==================================================
   VERSIONING
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
	u32 writer_pass; // R_GRAPH_INVALID_INDEX if initial
	u32 parent;      // R_GRAPH_INVALID_INDEX if initial
};


/* ==================================================
   IMPORTS
   ================================================== */

typedef struct R_GraphImportedTexture R_GraphImportedTexture;
struct R_GraphImportedTexture
{
	G_ResourceKey external_key;
	R_GraphTexHandle handle;
};

typedef struct R_GraphImportedBuffer R_GraphImportedBuffer;
struct R_GraphImportedBuffer
{
	G_ResourceKey external_key;
	R_GraphBufHandle handle;
};


/* ==================================================
   GRAPH
   ================================================== */

typedef struct R_Graph R_Graph;
struct R_Graph
{
	Arena *permanent_arena;
	
	LOG_Channel log_channel;

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
	VkFilter present_filter; // backbuffer is upscaled/downscaled to the swapchain size when presenting !
};


/* ==================================================
   CORE
   ================================================== */

internal void R_GraphInit    (R_Graph *graph, Arena *arena, LOG_Channel log_channel);
internal void R_GraphDestroy (R_Graph *graph);
internal void R_GraphReset   (R_Graph *graph);


/* ==================================================
   BUILDING
   ================================================== */

// --- Passes

internal R_Pass *R_GraphAdd(R_Graph *graph, String8 name, R_PassType type);


// --- Resources

internal R_GraphTexHandle R_GraphCreateTexture (R_Graph *graph, const R_TextureInfo *info);
internal R_GraphBufHandle R_GraphCreateBuffer  (R_Graph *graph, const R_BufferInfo *info);

internal R_GraphTexHandle R_GraphImportTexture (R_Graph *graph, G_ResourceKey external_key);
internal R_GraphBufHandle R_GraphImportBuffer  (R_Graph *graph, G_ResourceKey external_key);


// --- Versioning

internal R_GraphTexHandle R_GraphPushTexVersion (R_Graph *graph, R_GraphTexHandle parent, u32 writer_pass_index);
internal R_GraphBufHandle R_GraphPushBufVersion (R_Graph *graph, R_GraphBufHandle parent, u32 writer_pass_index);


/* ==================================================
   COMPILATION
   ================================================== */

internal void R_GraphCompile(R_Graph *graph, const G_Swapchain *swapchain);

internal void R_GraphPropagateDependencies     (R_Graph *graph);
internal void R_GraphBackpropagateDependencies (R_Graph *graph);
internal void R_GraphAllocateResources         (R_Graph *graph, const G_Swapchain *swapchain);
internal void R_GraphGenerateBarriers          (R_Graph *graph);

internal void R_GraphSyncTextureRead  (R_Graph *graph, R_Pass *pass, const R_PassTextureEdge *edge);
internal void R_GraphSyncTextureWrite (R_Graph *graph, R_Pass *pass, const R_PassTextureEdge *edge);

internal void R_GraphSyncBufferRead   (R_Graph *graph, R_Pass *pass, const R_PassBufferEdge *edge);
internal void R_GraphSyncBufferWrite  (R_Graph *graph, R_Pass *pass, const R_PassBufferEdge *edge);


/* ==================================================
   EXECUTION
   ================================================== */

internal void R_GraphExecute(R_Graph *graph, const G_Swapchain *swapchain, G_CmdBuffer *cmd);
internal void R_GraphPresentToSwapchain(R_Graph *graph, const G_Swapchain *swapchain, G_CmdBuffer *cmd);


/* ==================================================
   RESOURCE RESOLUTION
   ================================================== */

internal G_ResourceKey R_GraphResolveTexture     (const R_Graph *graph, R_GraphTexHandle handle);
internal G_ResourceKey R_GraphResolveTextureView (const R_Graph *graph, R_GraphTexHandle handle, G_SubresourceRange range);
internal G_ResourceKey R_GraphResolveBuffer      (const R_Graph *graph, R_GraphBufHandle handle);
internal R_BufferRange R_GraphResolveBufferRange (const R_Graph *graph, R_GraphBufHandle handle);


/* ==================================================
   HELPERS
   ================================================== */

internal void R_GraphSetBackbuffer(R_Graph *graph, R_GraphTexHandle handle);
internal void R_GraphSetPresentFilter(R_Graph *graph, VkFilter filter);

internal G_RenderInfo R_GraphBuildRenderingInfo(const R_Graph *graph, const R_Pass *pass);

internal R_GraphTexture *R_GraphTextureFromHandle (R_Graph *graph, R_GraphTexHandle handle);
internal R_GraphBuffer  *R_GraphBufferFromHandle  (R_Graph *graph, R_GraphBufHandle handle);

internal b32 R_GraphTexVersionIsUnwritten (const R_Graph *graph, R_GraphTexHandle handle);
internal b32 R_GraphBufVersionIsUnwritten (const R_Graph *graph, R_GraphBufHandle handle);


#endif // RENDER_GRAPH_H
