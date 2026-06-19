#ifndef GRAPHICS_CMD_BUFFER
#define GRAPHICS_CMD_BUFFER

#define G_ComputeGroupCount(count, tile) (((count) + (tile) - 1u) / (tile))

typedef struct G_Device G_Device;

typedef struct G_CmdBuffer G_CmdBuffer;
struct G_CmdBuffer
{
	VkCommandBuffer vk_handle;
	G_Device *device;
};

#define G_CmdInit(vk_handle, device) ((G_CmdBuffer) { (vk_handle), (device) })


/* ==================================================
   CORE
   ================================================== */

internal void G_CmdBegin(const G_CmdBuffer *cmd);
internal void G_CmdEnd(const G_CmdBuffer *cmd);


/* ==================================================
   DYNAMIC RENDERING
   ================================================== */

internal void G_CmdBeginRendering(const G_CmdBuffer *cmd, const G_RenderInfo *info);
internal void G_CmdEndRendering(const G_CmdBuffer *cmd);

internal void G_CmdSetViewport(const G_CmdBuffer *cmd, VkViewport viewport);
internal void G_CmdSetScissor(const G_CmdBuffer *cmd, VkRect2D scissor);


/* ==================================================
   SYNC
   ================================================== */

internal void G_CmdPipelineBarrier(const G_CmdBuffer *cmd, VkDependencyFlags dependency_flags,
									 u32 memory_barrier_count, const VkMemoryBarrier2 *memory_barriers,
									 u32 buffer_barrier_count, const VkBufferMemoryBarrier2 *buffer_barriers,
									 u32 image_barrier_count,  const VkImageMemoryBarrier2 *image_barriers);


/* ==================================================
   DESCRIPTORS
   ================================================== */

internal void G_CmdBindDescriptors(const G_CmdBuffer *cmd,
									 VkShaderStageFlags stage_flags,
									 G_PipelineLayoutKey layout, u32 first,
									 u32 descriptor_count, const VkDescriptorSet *descriptors,
									 u32 dynamic_offset_count, const u32 *dynamic_offsets);

internal void G_CmdBindBindless(const G_CmdBuffer *cmd,
								  VkShaderStageFlags stage_flags,
								  G_PipelineLayoutKey layout);


/* ==================================================
   PIPELINE
   ================================================== */

internal void G_CmdBindPipeline(const G_CmdBuffer *cmd,
								  VkPipelineBindPoint bind_point,
								  G_PipelineKey pipeline);


/* ==================================================
   INDEX BUFFER
   ================================================== */

internal void G_CmdBindIndexBuffer(const G_CmdBuffer *cmd,
									 G_BufferKey buffer,
									 u64 offset, u64 size, // VK_WHOLE_SIZE
									 VkIndexType type);


/* ==================================================
   PUSH CONSTANTS
   ================================================== */

internal void G_CmdPushConstants(const G_CmdBuffer *cmd,
								   G_PipelineLayoutKey layout,
								   VkShaderStageFlags stage_flags,
								   u64 size, const void *data,
								   u32 offset);


/* ==================================================
   DRAW
   ================================================== */

internal void G_CmdSetLineWidth(const G_CmdBuffer *cmd, f32 thickness);

internal void G_CmdDraw(const G_CmdBuffer *cmd,
						  u32 vertex_count,
						  u32 instance_count,
						  u32 first_vertex,
						  u32 first_instance);

#define G_CmdDrawV(cmd, vertex_count) G_CmdDraw((cmd), (vertex_count), 1, 0, 0)

internal void G_CmdDrawIndexed(const G_CmdBuffer *cmd,
								 u32 index_count,
								 u32 instance_count,
								 u32 first_index,
								 i32 vertex_offset,
								 u32 first_instance);

internal void G_CmdDrawIndexedIndirect(const G_CmdBuffer *cmd,
										 G_BufferKey buffer, u64 offset,
										 u32 count, u32 stride);

internal void G_CmdDrawIndexedIndirectCount(const G_CmdBuffer *cmd,
											  G_BufferKey indirect_buffer, u64 indirect_offset,
											  G_BufferKey count_buffer,    u64 count_offset,
											  u32 max_count, u32 stride);

internal void G_CmdDrawMeshTasksIndirectCount(const G_CmdBuffer *cmd,
												G_BufferKey indirect_buffer, u64 indirect_offset,
												G_BufferKey count_buffer,    u64 count_offset,
												u32 max_count, u32 stride);


/* ==================================================
   DISPATCH
   ================================================== */

internal void G_CmdDispatch(const G_CmdBuffer *cmd, u32 x, u32 y, u32 z);

internal void G_CmdDispatchIndirect(const G_CmdBuffer *cmd, G_BufferKey buffer, u64 offset);


/* ==================================================
   BLIT
   ================================================== */

internal void G_CmdBlit(const G_CmdBuffer *cmd,
						  G_TextureKey src,
						  G_TextureKey dst,
						  u32 region_count, const VkImageBlit2 *regions,
						  VkFilter filter);

internal void G_CmdGenerateMipmaps(const G_CmdBuffer *cmd, G_TextureKey texture);


/* ==================================================
   TRANSFER
   ================================================== */

typedef struct G_BufferCopy G_BufferCopy;
struct G_BufferCopy
{
	u64 src_offset;
	u64 dst_offset;
	u64 size;
};

typedef struct G_BufferImageCopy G_BufferImageCopy;
struct G_BufferImageCopy
{
	u64 buffer_offset;
	u32 buffer_row_length;
	u32 buffer_image_height;

	VkImageSubresourceLayers image_subresource;

	i32 x, y, z;
	u32 w, h, d;
};

internal void G_CmdCopyBufferToBuffer(const G_CmdBuffer *cmd,
										G_BufferKey src,
										G_BufferKey dst,
										u32 region_count, const G_BufferCopy *regions);

internal void G_CmdCopyBufferToTexture(const G_CmdBuffer *cmd,
										 G_BufferKey src,
										 G_TextureKey dst,
										 u32 region_count, const G_BufferImageCopy *regions);

internal void G_CmdCopyBufferToTextureWhole(const G_CmdBuffer *cmd,
											  G_BufferKey src,
											  G_TextureKey dst,
											  u64 buffer_offset);

internal void G_CmdFillBuffer(const G_CmdBuffer *cmd,
								G_BufferKey buffer,
								u64 offset, u64 size, u32 fill);


/* ==================================================
   ACCELERATION STRUCTURES
   ================================================== */

internal void G_CmdBuildBLAS(const G_CmdBuffer *cmd,
							   G_AccelStructKey blas,
							   const G_BLASGeometry *geometries, u32 geometry_count,
							   G_BufferKey scratch_buffer);

internal void G_CmdBuildTLAS(const G_CmdBuffer *cmd,
							   G_AccelStructKey tlas,
							   G_BufferKey instance_buffer, u32 instance_count,
							   G_BufferKey scratch_buffer);


/* ==================================================
   QUERY / PROFILING
   ================================================== */

internal void G_CmdBeginQuery(const G_CmdBuffer *cmd,
								VkQueryPool pool,
								u32 query, VkQueryControlFlags flags);

internal void G_CmdEndQuery(const G_CmdBuffer *cmd,
							  VkQueryPool pool,
							  u32 query);

internal void G_CmdResetQueries(const G_CmdBuffer *cmd,
								  VkQueryPool pool,
								  u32 first, u32 count);

internal void G_CmdWriteTimestamp(const G_CmdBuffer *cmd,
									VkPipelineStageFlags2 stage,
									VkQueryPool pool,
									u32 index);

internal void G_CmdBeginLabel(const G_CmdBuffer *cmd, String8 name);
internal void G_CmdBeginLabelEx(const G_CmdBuffer *cmd, String8 name, v4 colour);
internal void G_CmdEndLabel(const G_CmdBuffer *cmd);

#endif // GRAPHICS_CMD_BUFFER
