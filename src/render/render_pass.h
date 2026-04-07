#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#define R_PASS_MAX_I_TEXTURE_EDGES   16
#define R_PASS_MAX_O_TEXTURE_EDGES   16

#define R_PASS_MAX_I_BUFFER_EDGES    16
#define R_PASS_MAX_O_BUFFER_EDGES    16

#define R_PASS_MAX_MEMORY_BARRIERS   16
#define R_PASS_MAX_BUFFER_BARRIERS   16
#define R_PASS_MAX_TEXTURE_BARRIERS  16

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
	GFX_AccessSt access;
	VkImageLayout layout;
	GFX_SubresourceRange range;
	b32 should_clear;
	R_Clear clear;
};

typedef struct R_PassBufferEdge R_PassBufferEdge;
struct R_PassBufferEdge
{
	R_GraphBufHandle handle;
	GFX_AccessSt access;
	u64 offset;
	u64 size; // 0 = whole buffer.
};

typedef struct R_PassContext R_PassContext;
struct R_PassContext
{
	GFX_Device *device;	
	GFX_CmdBuffer *cmd;

	const R_Scene *scene;
	const R_Camera *camera;
	
	f32 delta_time;
	f32 elapsed_time;
};

typedef struct R_Graph R_Graph;

#define R_PASS_RECORD_SIG(fn) void fn(const R_Graph *graph, const R_PassContext *ctx, const void *user_data)
#define R_PASS_RECORD_DEF(fn) internal R_PASS_RECORD_SIG(fn)

typedef R_PASS_RECORD_SIG(R_PassRecordFn);

typedef struct R_Pass R_Pass;
struct R_Pass
{
	String8 name;
	R_PassType type;

	u32 index;
	b32 is_culled;

	R_PassRecordFn *record;
	const void *user_data;
	
	u32 multi_view_mask;

	// excuse the weird formatting but this reads nicer
	
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

internal R_GraphTexHandle R_PassAddInputTexture(R_Pass *pass, R_GraphTexHandle handle,
												VkPipelineStageFlags2 stage,
												VkAccessFlags2 access,
												VkImageLayout layout,
												GFX_SubresourceRange range);

internal R_GraphTexHandle R_PassAddOutputTexture(R_Pass *pass, R_GraphTexHandle handle,
												 VkPipelineStageFlags2 stage,
												 VkAccessFlags2 access,
												 VkImageLayout layout,
												 GFX_SubresourceRange range,
												 const R_Clear *clear);

internal R_GraphBufHandle R_PassAddInputBuffer(R_Pass *pass, R_GraphBufHandle handle,
											   VkPipelineStageFlags2 stage,
											   VkAccessFlags2 access);

internal R_GraphBufHandle R_PassAddOutputBuffer(R_Pass *pass, R_GraphBufHandle handle,
												VkPipelineStageFlags2 stage,
												VkAccessFlags2 access);


/* ==================================================
   TEXTURES
   ================================================== */

internal R_GraphTexHandle R_PassWriteColour(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear);
internal R_GraphTexHandle R_PassWriteDepth(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear);

internal R_GraphTexHandle R_PassWriteColourEx(R_Pass *pass, R_GraphTexHandle handle, GFX_SubresourceRange range, const R_Clear *clear);
internal R_GraphTexHandle R_PassWriteDepthEx(R_Pass *pass, R_GraphTexHandle handle, GFX_SubresourceRange range, const R_Clear *clear);

internal R_GraphTexHandle R_PassReadTexture(R_Pass *pass, R_GraphTexHandle handle);

internal R_GraphTexHandle R_PassReadTextureCompute(R_Pass *pass, R_GraphTexHandle handle);
internal R_GraphTexHandle R_PassWriteTextureCompute(R_Pass *pass, R_GraphTexHandle handle);

internal R_GraphTexHandle R_PassBlitTextureSrc(R_Pass *pass, R_GraphTexHandle handle);
internal R_GraphTexHandle R_PassBlitTextureDst(R_Pass *pass, R_GraphTexHandle handle);


/* ==================================================
   BUFFERS
   ================================================== */

internal R_GraphBufHandle R_PassWriteBufferGraphics(R_Pass *pass, R_GraphBufHandle handle);
internal R_GraphBufHandle R_PassReadBufferGraphics(R_Pass *pass, R_GraphBufHandle handle);

internal R_GraphBufHandle R_PassWriteBufferCompute(R_Pass *pass, R_GraphBufHandle handle);
internal R_GraphBufHandle R_PassReadBufferCompute(R_Pass *pass, R_GraphBufHandle handle);

internal R_GraphBufHandle R_PassIndirectBuffer(R_Pass *pass, R_GraphBufHandle handle);
internal R_GraphBufHandle R_PassClearBuffer(R_Pass *pass, R_GraphBufHandle handle);


#endif // RENDER_PASS_H
