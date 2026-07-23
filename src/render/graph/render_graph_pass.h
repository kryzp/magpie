#ifndef RENDER_GRAPH_PASS_H
#define RENDER_GRAPH_PASS_H

#define R_PASS_MAX_I_TEXTURE_EDGES   16
#define R_PASS_MAX_O_TEXTURE_EDGES   16

#define R_PASS_MAX_I_BUFFER_EDGES    16
#define R_PASS_MAX_O_BUFFER_EDGES    16

#define R_PASS_MAX_MEMORY_BARRIERS   16
#define R_PASS_MAX_BUFFER_BARRIERS   16
#define R_PASS_MAX_TEXTURE_BARRIERS  16

// TODO: Remove R_PassType. It's a basically depricated
//       thing and literally all it's ever used for is
//       automatically starting and ending CmdRendering
//       in the render graph. I'm keeping it right now
//       incase I ever need to know what kind of pass
//       it is but realistically I think this will have
//       to go in the future because it's just useless.

typedef enum R_PassType
{
	R_PassType_Graphics,
	R_PassType_Compute,
	R_PassType_Transfer,
	R_PassType_COUNT
}
R_PassType;

typedef struct R_PassTextureEdge R_PassTextureEdge;
struct R_PassTextureEdge
{
	R_GraphTexHandle handle;
	G_AccessSt state;
	VkImageLayout layout;
	G_SubresourceRange attachment_range;

	R_GraphTexHandle resolve_handle;
	VkImageLayout resolve_layout;
	VkResolveModeFlagBits resolve_mode;

	b32 should_clear;
	R_Clear clear;
};

typedef struct R_PassBufferEdge R_PassBufferEdge;
struct R_PassBufferEdge
{
	R_GraphBufHandle handle;
	
	G_AccessSt state;
	
	u64 offset;
	u64 size; // 0 = whole buffer.
};

typedef struct R_Graph R_Graph;

typedef struct R_PassContext R_PassContext;
struct R_PassContext
{
	R_Graph *graph;
	G_CmdBuffer *cmd;
	const G_RenderInfo *render_info;
	const void *user_data;
};

#define R_PASS_RECORD_DEF(fn) void fn(const R_PassContext *ctx)

typedef R_PASS_RECORD_DEF(R_PassRecordFn);

typedef struct R_Pass R_Pass;
struct R_Pass
{
	R_Graph *graph;
	
	String8 name;
	R_PassType type;
	u32 index;
	
	b32 is_culled;

	R_PassRecordFn *record;
	
	const void *user_data;
	
	u32 multi_view_mask;

	u32 input_texture_count;    R_PassTextureEdge input_textures  [R_PASS_MAX_I_TEXTURE_EDGES];
	u32 output_texture_count;   R_PassTextureEdge output_textures [R_PASS_MAX_O_TEXTURE_EDGES];

	u32 input_buffer_count;     R_PassBufferEdge input_buffers  [R_PASS_MAX_I_BUFFER_EDGES];
	u32 output_buffer_count;    R_PassBufferEdge output_buffers [R_PASS_MAX_O_BUFFER_EDGES];
	
	u32 memory_barrier_count;   VkMemoryBarrier2 memory_barriers       [R_PASS_MAX_MEMORY_BARRIERS];
	u32 buffer_barrier_count;   VkBufferMemoryBarrier2 buffer_barriers [R_PASS_MAX_BUFFER_BARRIERS];
	u32 texture_barrier_count;  VkImageMemoryBarrier2 texture_barriers [R_PASS_MAX_TEXTURE_BARRIERS];
};


/* ==================================================
   GENERAL
   ================================================== */

internal void R_PassSetRecord(R_Pass *pass, R_PassRecordFn *fn, const void *user_data);

internal void R_PassSetMultiViewMask(R_Pass *pass, u32 mask);


/* ==================================================
   HELPERS
   ================================================== */

internal R_GraphTexHandle R_PassAddInputTexture(R_Pass *pass,
												R_GraphTexHandle handle,
												VkPipelineStageFlags2 stage,
												VkAccessFlags2 access);

internal R_GraphTexHandle R_PassAddOutputTexture(R_Pass *pass,
												 R_GraphTexHandle handle,
												 const R_Clear *clear,
												 VkPipelineStageFlags2 stage,
												 VkAccessFlags2 access);

internal R_GraphBufHandle R_PassAddInputBuffer(R_Pass *pass, R_GraphBufHandle handle,
											   VkPipelineStageFlags2 stage,
											   VkAccessFlags2 access);

internal R_GraphBufHandle R_PassAddOutputBuffer(R_Pass *pass, R_GraphBufHandle handle,
												VkPipelineStageFlags2 stage,
												VkAccessFlags2 access);


/* ==================================================
   TEXTURES
   ================================================== */

internal R_GraphTexHandle R_PassWriteColour          (R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear);
internal R_GraphTexHandle R_PassWriteDepth           (R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear);

internal R_GraphTexHandle R_PassWriteColourEx        (R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear, G_SubresourceRange range);
internal R_GraphTexHandle R_PassWriteDepthEx         (R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear, G_SubresourceRange range);

internal R_GraphTexHandle R_PassWriteColourResolve   (R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear);
internal R_GraphTexHandle R_PassWriteDepthResolve    (R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear);

internal R_GraphTexHandle R_PassWriteColourResolveEx (R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear, G_SubresourceRange range);
internal R_GraphTexHandle R_PassWriteDepthResolveEx  (R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear, G_SubresourceRange range);

internal R_GraphTexHandle R_PassReadTextureGraphics  (R_Pass *pass, R_GraphTexHandle handle);

internal R_GraphTexHandle R_PassReadTextureCompute   (R_Pass *pass, R_GraphTexHandle handle);
internal R_GraphTexHandle R_PassWriteTextureCompute  (R_Pass *pass, R_GraphTexHandle handle);

internal R_GraphTexHandle R_PassBlitTextureSrc       (R_Pass *pass, R_GraphTexHandle handle);
internal R_GraphTexHandle R_PassBlitTextureDst       (R_Pass *pass, R_GraphTexHandle handle);


/* ==================================================
   BUFFERS
   ================================================== */

internal R_GraphBufHandle R_PassWriteBufferGraphics  (R_Pass *pass, R_GraphBufHandle handle);
internal R_GraphBufHandle R_PassReadBufferGraphics   (R_Pass *pass, R_GraphBufHandle handle);

internal R_GraphBufHandle R_PassWriteBufferCompute   (R_Pass *pass, R_GraphBufHandle handle);
internal R_GraphBufHandle R_PassReadBufferCompute    (R_Pass *pass, R_GraphBufHandle handle);

internal R_GraphBufHandle R_PassIndirectBuffer       (R_Pass *pass, R_GraphBufHandle handle);
internal R_GraphBufHandle R_PassClearBuffer          (R_Pass *pass, R_GraphBufHandle handle);


#endif // RENDER_GRAPH_PASS_H
