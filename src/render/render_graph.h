#ifndef RENDER_GRAPH_H
#define RENDER_GRAPH_H

typedef enum R_SizeClass
{
	R_SizeClass_Absolute,
	R_SizeClass_SwapchainRelative,
	R_SizeClass_COUNT
}
R_SizeClass;

typedef struct R_AttachmentInfo R_AttachmentInfo;
struct R_AttachmentInfo
{
	VkFormat format;

	R_SizeClass size_class;
	f32 size_x, size_y, size_z;

	u32 mips;
	u32 layers;

	u32 samples;

	b32 is_cubemap;
	b32 is_transient;
	b32 is_storage;
};

b32 R_AttachmentInfoMatch(const R_AttachmentInfo *a, const R_AttachmentInfo *b);

typedef struct R_BufferInfo R_BufferInfo;
struct R_BufferInfo
{
	VkDeviceSize size;
	VmaAllocationCreateFlags flags;
	VkBufferUsageFlags2 usage;
};

b32 R_BufferInfoMatch(const R_BufferInfo *a, const R_BufferInfo *b);

typedef union R_Clear R_Clear;
union R_Clear
{
	struct
	{
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	}
	colour;

	struct
	{
		f32 depth;
		u8 stencil;
	};
};

typedef struct R_GraphHandle R_GraphHandle;
struct R_GraphHandle
{
	u32 value;
};

typedef struct R_GraphEdge R_GraphEdge;
struct R_GraphEdge
{
	R_GraphHandle handle;
	GFX_AccessSt access_st;
	GFX_SubresourceRange range;
	b32 clear_enabled;
	R_Clear clear;
};

typedef struct R_GraphResourceState R_GraphResourceState;
struct R_GraphResourceState
{
	VkPipelineStageFlags2 pipeline_barrier_stage_flags;

	// Access masks that need to be made available.
	VkAccessFlags2 to_flush_access;

	// Bitmask of access flags that have been made visible to a specific pipeline.
	VkAccessFlags2 invalidated_in_stage[64];

	// For texture tracking.
	// We only use VK_IMAGE_LAYOUT_GENERAL pretty much, but textures
	// initially start off in VK_IMAGE_LAYOUT_UNDEFINED, so we need
	// to track it.
	// TOOD: Can this be simplified / removed?
	VkImageLayout layout;
};

typedef enum R_GraphResourceKind
{
	R_GraphResourceKind_Attachment,
	R_GraphResourceKind_Buffer,
	R_GraphResourceKind_COUNT
}
R_GraphResourceKind;

typedef struct R_GraphResource R_GraphResource;
struct R_GraphResource
{
	R_GraphResourceKind kind;

	b32 is_imported;

	u32 first_stage_index;
	u32 last_stage_index;

	u32 ref_count;

	R_GraphResourceKind state;

	GFX_TextureKey physical_texture;
	R_AttachmentInfo texture_info;

	GFX_BufferKey physical_buffer;
	R_BufferInfo buffer_info;
	u64 buffer_offset; // TODO: Should this be removed?
};

typedef struct R_GraphBlackboard R_GraphBlackboard;
struct R_GraphBlackboard
{
	Arena *arena;
};

void R_GraphBlackboardClean(R_GraphBlackboard *bb);

void *R_GraphBlackboardAdd(R_GraphBlackboard *bb, u32 id, void *data, u64 size);
void *R_GraphBlackboardGet(R_GraphBlackboard *bb, u32 id);

typedef struct R_GraphContext R_GraphContext;
struct R_GraphContext
{
	GFX_Device *device;
	GFX_CmdBuffer *cmd;

	const R_Scene *scene;
	const R_Camera *camera;
	
	f32 delta_time;
	f32 elapsed_time;
};

typedef struct R_Graph R_Graph;
typedef struct R_Stage R_Stage;

typedef struct R_StageResources R_StageResources;
struct R_StageResources
{
	R_Graph *graph;
	R_Stage *stage;
};

GFX_RenderInfo R_StageResRenderInfo(const R_StageResources *resources);

const GFX_Texture *R_StageResTexture(const R_StageResources *resources, R_GraphHandle handle);
const GFX_TextureView *R_StageResTextureView(const R_StageResources *resources, R_GraphHandle handle, GFX_SubresourceRange range);

const GFX_Buffer *R_StageResBuffer(const R_StageResources *resources, R_GraphHandle handle);

// The view is NOT the same as saying get_buffer(handle)->get_device_address()!!
// This also applies the physical_offset to the output.
// Required for situations where multiple resources lie on different sections of a physical buffer.		
GFX_BufferRange R_StageResBufferRange(const R_StageResources *resources, R_GraphHandle handle);

typedef enum R_StageType
{
	R_StageType_Graphics,
	R_StageType_Compute,
	R_StageType_Transfer,
	R_StageType_COUNT
}
R_StageType;

#define R_GRAPH_RECORD_SIG(fn) void fn(const R_GraphContext *ctx, const R_StageResources *resources)
#define R_GRAPH_RECORD_DEF(fn) internal R_GRAPH_RECORD_SIG(fn)

typedef R_GRAPH_RECORD_SIG(R_GraphRecordFn);

typedef struct R_Stage R_Stage;
struct R_Stage
{
	String8 name;
	R_StageType type;
	u32 index;

	R_GraphRecordFn *record;
	
	u32 multi_view_mask;

	b32 is_culled;

	u32 input_count;
	R_GraphEdge inputs[16];

	u32 output_count;
	R_GraphEdge outputs[16];

	u32 memory_barrier_count;
	VkMemoryBarrier2 memory_barriers[16];

	u32 buffer_barrier_count;
	VkBufferMemoryBarrier2 buffer_barriers[16];

	u32 texture_barrier_count;
	VkImageMemoryBarrier2 texture_barriers[16];
};

internal void R_StageAddEdge(R_Stage *stage, R_GraphHandle handle,
							 GFX_AccessSt state,
							 GFX_SubresourceRange range,
							 const R_Clear *clear,
							 b32 is_output);

internal void R_StageSetRecord(R_Stage *stage, R_GraphRecordFn *fn);
internal void R_StageSetMultiViewMask(R_Stage *stage, u32 mask);

internal R_GraphHandle R_StageWriteColour(R_Stage *stage, R_GraphHandle handle, const R_Clear *clear);
internal R_GraphHandle R_StageWriteDepth(R_Stage *stage, R_GraphHandle handle, const R_Clear *clear);

internal R_GraphHandle R_StageWriteColourEx(R_Stage *stage, R_GraphHandle handle, GFX_SubresourceRange range, const R_Clear *clear);
internal R_GraphHandle R_StageWriteDepthEx(R_Stage *stage, R_GraphHandle handle, GFX_SubresourceRange range, const R_Clear *clear);

internal R_GraphHandle R_StageReadTexture(R_Stage *stage, R_GraphHandle handle);

internal R_GraphHandle R_StageReadTextureCompute(R_Stage *stage, R_GraphHandle handle);
internal R_GraphHandle R_StageWriteTextureCompute(R_Stage *stage, R_GraphHandle handle);

internal R_GraphHandle R_StageBlitTextureSrc(R_Stage *stage, R_GraphHandle handle);
internal R_GraphHandle R_StageBlitTextureDst(R_Stage *stage, R_GraphHandle handle);

internal R_GraphHandle R_StageWriteBufferGraphics(R_Stage *stage, R_GraphHandle handle);
internal R_GraphHandle R_StageReadBufferGraphics(R_Stage *stage, R_GraphHandle handle);

internal R_GraphHandle R_StageWriteBufferCompute(R_Stage *stage, R_GraphHandle handle);
internal R_GraphHandle R_StageReadBufferCompute(R_Stage *stage, R_GraphHandle handle);

internal R_GraphHandle R_StageIndirectBuffer(R_Stage *stage, R_GraphHandle handle);
internal R_GraphHandle R_StageClearBuffer(R_Stage *stage, R_GraphHandle handle);

typedef struct R_GraphPooledTexture R_GraphPooledTexture;
struct R_GraphPooledTexture
{
	R_GraphPooledTexture *next;

	GFX_TextureKey physical_texture;
	R_AttachmentInfo info;
	b32 in_use;
	u64 last_time_used;
	R_GraphResourceState state;
};

typedef struct R_GraphPooledBuffer R_GraphPooledBuffer;
struct R_GraphPooledBuffer
{
	R_GraphPooledBuffer *next;

	GFX_BufferKey physical_buffer;
	R_BufferInfo info;
	u32 in_use;
	u64 last_time_used;
	R_GraphResourceState state;
};

typedef struct R_GraphResourcePool R_GraphResourcePool;
struct R_GraphResourcePool
{
	u64 current_time;
	u64 gpu_completed_time;

	// TODO: Convert into freelist.
	R_GraphPooledTexture *texture_pool_head;
	R_GraphPooledBuffer  *buffer_pool_head;
};

void GFX_GraphResourcePoolInit(R_GraphResourcePool *pool);
void GFX_GraphResourcePoolDestroy(R_GraphResourcePool *pool);
void GFX_GraphResourcePoolFlush(R_GraphResourcePool *pool);

const GFX_Texture *GFX_GraphResourcePoolAcquireTexture(R_GraphResourcePool *pool,
													   const R_AttachmentInfo *info,
													   R_GraphResourceState *out_state);

const GFX_Buffer *GFX_GraphResourcePoolAcquireBuffer(R_GraphResourcePool *pool,
													 const R_BufferInfo *info,
													 R_GraphResourceState *out_state);

void GFX_GraphResourcePoolUpdateTexture(R_GraphResourcePool *pool,
										const GFX_Texture *texture,
										const R_GraphResourceState *state);

void GFX_GraphResourcePoolUpdateBuffer(R_GraphResourcePool *pool,
									   const GFX_Buffer *buffer,
									   const R_GraphResourceState *state);

typedef struct R_GraphImportedResource R_GraphImportedResource;
struct R_GraphImportedResource
{
	R_GraphImportedResource *next;
	u64 key;
	R_GraphHandle handle;
};

typedef struct R_GraphTrackedTexture R_GraphTrackedTexture;
struct R_GraphTrackedTexture
{
	R_GraphTrackedTexture *next;
	GFX_TextureKey key;
	R_GraphResourceState state;
};

typedef struct R_GraphTrackedBuffer R_GraphTrackedBuffer;
struct R_GraphTrackedBuffer
{
	R_GraphTrackedTexture *next;
	GFX_BufferKey key;
	R_GraphResourceState state;
};

typedef struct R_Graph R_Graph;
struct R_Graph
{
	Arena *arena;
	
	u32 resource_count;
	R_GraphResource resources[512];

	u32 stage_count;
	R_Stage stages[64];
	
	R_GraphResourcePool pool;
	R_GraphHandle backbuffer_handle;

	R_GraphImportedResource *import_cache_head;

	R_GraphTrackedTexture *tracked_texture_head;
	R_GraphTrackedBuffer  *tracked_buffer_head;
};

internal void R_GraphInit(R_Graph *graph, Arena *arena);
internal void R_GraphDestroy(R_Graph *graph);

internal void R_GraphReset(R_Graph *graph);

internal R_Stage *R_GraphPush(String8 name, R_StageType type);

internal void R_GraphSetBackbuffer(R_Graph *graph, R_GraphHandle handle);

internal void R_GraphPropogateDependencies     (R_Graph *graph);
internal void R_GraphBackpropogateDependencies (R_Graph *graph);
internal void R_GraphAllocateResources         (R_Graph *graph, const GFX_Swapchain *swapchain);
internal void R_GraphGenerateBarriers          (R_Graph *graph);

internal void R_GraphProcessInvalidate (R_Graph *graph, R_Stage *stage, const R_GraphEdge *edge);
internal void R_GraphProcessFlush      (R_Graph *graph, R_Stage *stage, const R_GraphEdge *edge);

internal void R_GraphPresentToSwapchain(R_Graph *graph,
										const GFX_Swapchain *swapchain,
										const GFX_CmdBuffer *cmd);

internal void R_GraphCompile(R_Graph *graph,
							 const GFX_Swapchain *swapchain);

internal void R_GraphExecute(R_Graph *graph,
							 const GFX_Swapchain *swapchain,
							 const GFX_CmdBuffer *cmd,
							 const R_Scene *scene,
							 const R_Camera *camera,
							 f32 delta_time, f32 elapsed_time);

R_GraphHandle R_GraphCreateTexture (R_Graph *graph, const R_AttachmentInfo *info);
R_GraphHandle R_GraphCreateBuffer  (R_Graph *graph, const R_BufferInfo *info);

R_GraphHandle R_GraphImportTexture (R_Graph *graph, GFX_TextureKey external_texture);
R_GraphHandle R_GraphImportBuffer  (R_Graph *graph, GFX_BufferKey external_buffer);

#endif // RENDER_GRAPH_H
