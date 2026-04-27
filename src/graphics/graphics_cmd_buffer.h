#ifndef GRAPHICS_CMD_BUFFER
#define GRAPHICS_CMD_BUFFER

#define GFX_ComputeGroupCount(count, tile) (((count) + (tile) - 1u) / (tile))

typedef struct GFX_Device GFX_Device;

typedef struct GFX_CmdBuffer GFX_CmdBuffer;
struct GFX_CmdBuffer
{
	VkCommandBuffer handle;
	GFX_Device *device;
};

#define GFX_CmdInit(vk_handle, device) ((GFX_CmdBuffer) { (vk_handle), (device) })

internal void GFX_CmdBegin(const GFX_CmdBuffer *cmd);
internal void GFX_CmdEnd(const GFX_CmdBuffer *cmd);

internal void GFX_CmdBeginRendering(const GFX_CmdBuffer *cmd, const GFX_RenderInfo *info);
internal void GFX_CmdEndRendering(const GFX_CmdBuffer *cmd);

internal void GFX_CmdSetViewport(const GFX_CmdBuffer *cmd, VkViewport viewport);
internal void GFX_CmdSetScissor(const GFX_CmdBuffer *cmd, VkRect2D scissor);

internal void GFX_CmdPipelineBarrier(const GFX_CmdBuffer *cmd, VkDependencyFlags dependency_flags,
									 u32 memory_barrier_count, const VkMemoryBarrier2 *memory_barriers,
									 u32 buffer_barrier_count, const VkBufferMemoryBarrier2 *buffer_barriers,
									 u32 image_barrier_count,  const VkImageMemoryBarrier2 *image_barriers);

internal void GFX_CmdBindDescriptors(const GFX_CmdBuffer *cmd,
									 VkShaderStageFlags stage_flags,
									 GFX_PipelineLayoutKey layout, u32 first,
									 u32 descriptor_count, const VkDescriptorSet *descriptors,
									 u32 dynamic_offset_count, const u32 *dynamic_offsets);

internal void GFX_CmdBindBindless(const GFX_CmdBuffer *cmd,
								  VkShaderStageFlags stage_flags,
								  GFX_PipelineLayoutKey layout);

internal void GFX_CmdBindPipeline(const GFX_CmdBuffer *cmd,
								  VkPipelineBindPoint bind_point,
								  GFX_PipelineKey pipeline);

internal void GFX_CmdBindIndexBuffer(const GFX_CmdBuffer *cmd,
									 GFX_BufferKey buffer,
									 u64 offset, u64 size, // VK_WHOLE_SIZE
									 VkIndexType type);

internal void GFX_CmdPushConstants(const GFX_CmdBuffer *cmd,
								   GFX_PipelineLayoutKey layout,
								   VkShaderStageFlags stage_flags,
								   u64 size, const void *data,
								   u32 offset);

internal void GFX_CmdSetLineWidth(const GFX_CmdBuffer *cmd, f32 thickness);

internal void GFX_CmdDraw(const GFX_CmdBuffer *cmd,
						  u32 vertex_count,
						  u32 instance_count,
						  u32 first_vertex,
						  u32 first_instance);

#define GFX_CmdDrawV(cmd, vertex_count) GFX_CmdDraw((cmd), (vertex_count), 1, 0, 0)

internal void GFX_CmdDrawIndexed(const GFX_CmdBuffer *cmd,
								 u32 index_count,
								 u32 instance_count,
								 u32 first_index,
								 i32 vertex_offset,
								 u32 first_instance);

internal void GFX_CmdDrawIndexedIndirect(const GFX_CmdBuffer *cmd,
										 GFX_BufferKey buffer, u64 offset,
										 u32 count, u32 stride);

internal void GFX_CmdDrawIndexedIndirectCount(const GFX_CmdBuffer *cmd,
											  GFX_BufferKey indirect_buffer, u64 indirect_offset,
											  GFX_BufferKey count_buffer,    u64 count_offset,
											  u32 max_count, u32 stride);

internal void GFX_CmdDrawMeshTasksIndirectCount(const GFX_CmdBuffer *cmd,
												GFX_BufferKey indirect_buffer, u64 indirect_offset,
												GFX_BufferKey count_buffer,    u64 count_offset,
												u32 max_count, u32 stride);

internal void GFX_CmdDispatch(const GFX_CmdBuffer *cmd, u32 x, u32 y, u32 z);

internal void GFX_CmdDispatchIndirect(const GFX_CmdBuffer *cmd, GFX_BufferKey buffer, u64 offset);

internal void GFX_CmdBlit(const GFX_CmdBuffer *cmd,
						  GFX_TextureKey src,
						  GFX_TextureKey dst,
						  u32 region_count, const VkImageBlit2 *regions,
						  VkFilter filter);

internal void GFX_CmdGenerateMipmaps(const GFX_CmdBuffer *cmd, GFX_TextureKey texture);

internal void GFX_CmdCopyBufferToBuffer(const GFX_CmdBuffer *cmd,
										GFX_BufferKey src,
										GFX_BufferKey dst,
										u32 region_count, const VkBufferCopy2 *regions);

internal void GFX_CmdCopyBufferToTexture(const GFX_CmdBuffer *cmd,
										 GFX_BufferKey src,
										 GFX_TextureKey dst,
										 u32 region_count, const VkBufferImageCopy2 *regions);

internal void GFX_CmdCopyBufferToTextureWhole(const GFX_CmdBuffer *cmd,
											  GFX_BufferKey src,
											  GFX_TextureKey dst,
											  u64 buffer_offset);

internal void GFX_CmdFillBuffer(const GFX_CmdBuffer *cmd,
								GFX_BufferKey buffer,
								u64 offset, u64 size, u32 fill);

internal void GFX_CmdBeginQuery(const GFX_CmdBuffer *cmd,
								VkQueryPool pool,
								u32 query, VkQueryControlFlags flags);

internal void GFX_CmdEndQuery(const GFX_CmdBuffer *cmd,
							  VkQueryPool pool,
							  u32 query);

internal void GFX_CmdResetQueries(const GFX_CmdBuffer *cmd,
								  VkQueryPool pool,
								  u32 first, u32 count);

internal void GFX_CmdWriteTimestamp(const GFX_CmdBuffer *cmd,
									VkPipelineStageFlags2 stage,
									VkQueryPool pool,
									u32 index);

#endif // GRAPHICS_CMD_BUFFER
