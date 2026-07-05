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
	G_TextureKey physical_key;
	R_TextureInfo texture_info;
	
	u32 first_pass_index;
	u32 last_pass_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	G_TextureKey imported_key;
};

typedef struct R_GraphBuffer R_GraphBuffer;
struct R_GraphBuffer
{
	G_BufferKey physical_key;
	R_BufferInfo buffer_info;
	
	u32 first_pass_index;
	u32 last_pass_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	G_BufferKey imported_key;
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
	G_TextureKey external_key;
	R_GraphTexHandle handle;
};

typedef struct R_GraphImportedBuffer R_GraphImportedBuffer;
struct R_GraphImportedBuffer
{
	G_BufferKey external_key;
	R_GraphBufHandle handle;
};


/* ==================================================
   GRAPH
   ================================================== */

typedef struct R_Graph R_Graph;
struct R_Graph
{
	Arena *permanent_arena;
	
	G_Device *device;

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

static void R_GraphInit    (R_Graph *graph, Arena *arena, G_Device *device, LOG_Channel log_channel);
static void R_GraphDestroy (R_Graph *graph);
static void R_GraphReset   (R_Graph *graph);


/* ==================================================
   BUILDING
   ================================================== */

// --- Passes

static R_Pass *R_GraphAdd(R_Graph *graph, String8 name, R_PassType type);


// --- Resources

static R_GraphTexHandle R_GraphCreateTexture (R_Graph *graph, const R_TextureInfo *info);
static R_GraphBufHandle R_GraphCreateBuffer  (R_Graph *graph, const R_BufferInfo *info);

static R_GraphTexHandle R_GraphImportTexture (R_Graph *graph, G_TextureKey external_key);
static R_GraphBufHandle R_GraphImportBuffer  (R_Graph *graph, G_BufferKey external_key);


// --- Versioning

static R_GraphTexHandle R_GraphPushTexVersion (R_Graph *graph, R_GraphTexHandle parent, u32 writer_pass_index);
static R_GraphBufHandle R_GraphPushBufVersion (R_Graph *graph, R_GraphBufHandle parent, u32 writer_pass_index);


/* ==================================================
   COMPILATION
   ================================================== */

static void R_GraphCompile(R_Graph *graph, const G_Swapchain *swapchain);

static void R_GraphPropagateDependencies     (R_Graph *graph);
static void R_GraphBackpropagateDependencies (R_Graph *graph);
static void R_GraphAllocateResources         (R_Graph *graph, const G_Swapchain *swapchain);
static void R_GraphGenerateBarriers          (R_Graph *graph);

static void R_GraphSyncTextureRead  (R_Graph *graph, R_Pass *pass, const R_PassTextureEdge *edge);
static void R_GraphSyncTextureWrite (R_Graph *graph, R_Pass *pass, const R_PassTextureEdge *edge);

static void R_GraphSyncBufferRead   (R_Graph *graph, R_Pass *pass, const R_PassBufferEdge *edge);
static void R_GraphSyncBufferWrite  (R_Graph *graph, R_Pass *pass, const R_PassBufferEdge *edge);


/* ==================================================
   EXECUTION
   ================================================== */

static void R_GraphExecute(R_Graph *graph, const G_Swapchain *swapchain, G_CmdBuffer *cmd);
static void R_GraphPresentToSwapchain(R_Graph *graph, const G_Swapchain *swapchain, G_CmdBuffer *cmd);


/* ==================================================
   RESOURCE RESOLUTION
   ================================================== */

static G_TextureKey     R_GraphResolveTexture     (const R_Graph *graph, R_GraphTexHandle handle);
static G_TextureViewKey R_GraphResolveTextureView (const R_Graph *graph, R_GraphTexHandle handle, G_SubresourceRange range);
static G_BufferKey      R_GraphResolveBuffer      (const R_Graph *graph, R_GraphBufHandle handle);
static R_BufferRange    R_GraphResolveBufferRange (const R_Graph *graph, R_GraphBufHandle handle);


/* ==================================================
   HELPERS
   ================================================== */

static void R_GraphSetBackbuffer(R_Graph *graph, R_GraphTexHandle handle);
static void R_GraphSetPresentFilter(R_Graph *graph, VkFilter filter);

static G_RenderInfo R_GraphBuildRenderingInfo(const R_Graph *graph, const R_Pass *pass);

static R_GraphTexture *R_GraphTextureFromHandle (R_Graph *graph, R_GraphTexHandle handle);
static R_GraphBuffer  *R_GraphBufferFromHandle  (R_Graph *graph, R_GraphBufHandle handle);

static b32 R_GraphTexVersionIsUnwritten (const R_Graph *graph, R_GraphTexHandle handle);
static b32 R_GraphBufVersionIsUnwritten (const R_Graph *graph, R_GraphBufHandle handle);


/* ==================================================
   MSAA
   ================================================== */

static R_GraphMsaaTexture R_GraphCreateMsaa(R_Graph *graph, const R_TextureInfo *base, VkSampleCountFlagBits samples);


#endif // RENDER_GRAPH_H
