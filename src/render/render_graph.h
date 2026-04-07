#ifndef RENDER_GRAPH_H
#define RENDER_GRAPH_H

#define R_GRAPH_MAX_TEXTURES  256
#define R_GRAPH_MAX_BUFFERS   256
#define R_GRAPH_MAX_PASSES    64
#define R_GRAPH_MAX_IMPORTS   64
#define R_GRAPH_MAX_TRACKED   64


/* ==================================================
   VIRTUAL RESOURCES

   Basically here's how render graphs work - We keep
   an internal representation of a "resource" which
   we use to describe passes, etc... But we only give
   it physical backing at the end - because only by
   the end of the graph do we have a full understanding
   of what capabilities that resource needs to have,
   and the different state changes it needs to undergo.
   ================================================== */

typedef struct R_GraphTexture R_GraphTexture;
struct R_GraphTexture
{
	R_TextureInfo texture_info;
	GFX_TextureKey physical_texture;
	
	u32 first_stage_index;
	u32 last_stage_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	GFX_TextureKey imported_key;
};

typedef struct R_GraphBuffer R_GraphBuffer;
struct R_GraphBuffer
{
	R_BufferInfo buffer_info;
	GFX_BufferKey physical_buffer;
	
	u32 first_stage_index;
	u32 last_stage_index;
	u32 ref_count;

	R_ResourceState state;

	b32 is_imported;
	GFX_BufferKey imported_key;
};


/* ==================================================
   IMPORTS

   The render graph needs to be able to support "importing"
   textures that live externally outside of the render
   graph, so we also need to track them and such.
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
   CROSS-FRAME TRACKING

   Imported resources might have their states change
   between graph resets so we need to restore them
   on the next import.

   TODO: This fucking sucks balls and I should remove
         this bullshit garbage bug-prone feature.
   ================================================== */

typedef struct R_GraphTrackedTexture R_GraphTrackedTexture;
struct R_GraphTrackedTexture
{
	GFX_TextureKey key;
	R_ResourceState state;
};

typedef struct R_GraphTrackedBuffer R_GraphTrackedBuffer;
struct R_GraphTrackedBuffer
{
	GFX_BufferKey key;
	R_ResourceState state;
};


/* ==================================================
   GRAPH
   ================================================== */

typedef struct R_Graph R_Graph;
struct R_Graph
{
	Arena *permanent_arena;
	Arena *frame_arena;

	// Virtual Resources
	u32 texture_count;
	R_GraphTexture textures[R_GRAPH_MAX_TEXTURES];

	u32 buffer_count;
	R_GraphBuffer buffers[R_GRAPH_MAX_BUFFERS];

	// Passes
	u32 pass_count;
	R_Pass passes[R_GRAPH_MAX_PASSES];

	// Pool
	R_ResourcePool pool;

	// Final output texture to present.
	R_GraphTexHandle backbuffer_handle;

	// Importing
	u32 imported_texture_count;
	R_GraphImportedTexture imported_textures[R_GRAPH_MAX_IMPORTS];
	
	u32 imported_buffers_count;
	R_GraphImportedBuffer imported_buffers[R_GRAPH_MAX_IMPORTS];

	// Cross-Frame Tracking
	u32 tracked_texture_count;
	R_GraphTrackedTexture tracked_textures[R_GRAPH_MAX_TRACKED];

	u32 tracked_buffer_count;
	R_GraphTrackedBuffer tracked_buffers[R_GRAPH_MAX_TRACKED];
};


/* ==================================================
   CORE
   ================================================== */

internal void R_GraphInit(R_Graph *graph, Arena *permanent_arena, Arena *frame_arena);
internal void R_GraphDestroy(R_Graph *graph);

internal void R_GraphReset(R_Graph *graph);


/* ==================================================
   BUILDING
   ================================================== */

internal R_Pass *R_GraphPush(R_Graph *graph, String8 name, R_PassType type);

internal void R_GraphSetBackbuffer(R_Graph *graph, R_GraphTexHandle handle);

internal R_GraphTexHandle R_GraphCreateTexture (R_Graph *graph, const R_TextureInfo *info);
internal R_GraphBufHandle R_GraphCreateBuffer  (R_Graph *graph, const R_BufferInfo *info);

internal R_GraphTexHandle R_GraphImportTexture (R_Graph *graph, const GFX_Device *device, GFX_TextureKey external_key);
internal R_GraphBufHandle R_GraphImportBuffer  (R_Graph *graph, const GFX_Device *device, GFX_BufferKey external_key);


/* ==================================================
   COMPILATION
   ================================================== */

// --- Main API

internal void R_GraphCompile(R_Graph *graph,
							 const GFX_Swapchain *swapchain);


// --- Helpers

internal void R_GraphPropogateDependencies     (R_Graph *graph);
internal void R_GraphBackpropogateDependencies (R_Graph *graph);
internal void R_GraphAllocateResources         (R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain);
internal void R_GraphGenerateBarriers          (R_Graph *graph, const GFX_Device *device);

internal void R_GraphProcessInvalidateTexture (R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassTextureEdge *edge);
internal void R_GraphProcessInvalidateBuffer  (R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassBufferEdge *edge);

internal void R_GraphProcessFlushTexture (R_Graph *graph, const R_PassTextureEdge *edge);
internal void R_GraphProcessFlushBuffer  (R_Graph *graph, const R_PassBufferEdge *edge);


/* ==================================================
   EXECUTION
   ================================================== */

// --- Main API

internal void R_GraphExecute(R_Graph *graph,
							 const GFX_Swapchain *swapchain,
							 const GFX_CmdBuffer *cmd,
							 const R_Scene *scene,
							 const R_Camera *camera,
							 f32 delta_time, f32 elapsed_time);


// --- Helpers

internal void R_GraphPresentToSwapchain(R_Graph *graph,
										const GFX_Swapchain *swapchain,
										const GFX_CmdBuffer *cmd);


/* ==================================================
   RESOURCES
   ================================================== */

internal const GFX_Texture *R_GraphResolveTexture(const R_Graph *graph, const GFX_Device *device,
												  R_GraphTexHandle handle);

internal const GFX_TextureView *R_GraphResolveTextureView(const R_Graph *graph, const GFX_Device *device,
														  R_GraphTexHandle key, GFX_SubresourceRange range);

internal const GFX_Buffer *R_GraphResolveBuffer(const R_Graph *graph, const GFX_Device *device,
												R_GraphBufHandle key);

// DeviceAddress(BufferRange) != DeviceAddress(BufferRange->Buffer)
internal GFX_BufferRange R_GraphResolveBufferRange(const R_Graph *graph, const GFX_Device *device,
												   R_GraphBufHandle key);


/* ==================================================
   HELPERS
   ================================================== */

internal GFX_RenderInfo R_GraphBuildRenderingInfo(const R_Graph *graph, const GFX_Device *device, const R_Pass *pass);


#endif // RENDER_GRAPH_H
