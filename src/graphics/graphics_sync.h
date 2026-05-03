#ifndef GRAPHICS_SYNC_H
#define GRAPHICS_SYNC_H

typedef struct GFX_AccessSt GFX_AccessSt;
struct GFX_AccessSt
{
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
};

#define GFX_SYNC_WRITE_ACCESS_MASK						\
	(VK_ACCESS_2_SHADER_WRITE_BIT |						\
	 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |			\
	 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |	\
	 VK_ACCESS_2_TRANSFER_WRITE_BIT |					\
	 VK_ACCESS_2_HOST_WRITE_BIT |						\
	 VK_ACCESS_2_MEMORY_WRITE_BIT |						\
	 VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)

internal VkMemoryBarrier2 GFX_SyncMemoryBarrier(const GFX_AccessSt *src,
												const GFX_AccessSt *dst);

internal VkImageMemoryBarrier2 GFX_SyncTextureBarrier(const GFX_Texture *texture,
													  const GFX_AccessSt *src,
													  const GFX_AccessSt *dst,
													  VkImageLayout src_layout,
													  VkImageLayout dst_layout,
													  u32 base_mip,   u32 mip_count,    // VK_REMAINING_MIP_LEVELS
													  u32 base_layer, u32 layer_count); // VK_REMAINING_ARRAY_LAYERS

internal VkBufferMemoryBarrier2 GFX_SyncBufferBarrier(const GFX_Buffer *buffer,
													  const GFX_AccessSt *src,
													  const GFX_AccessSt *dst,
													  u64 offset, u64 size); // VK_WHOLE_SIZE

#endif // GRAPHICS_SYNC_H
