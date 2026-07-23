#ifndef GRAPHICS_SYNC_H
#define GRAPHICS_SYNC_H

typedef struct G_AccessSt G_AccessSt;
struct G_AccessSt
{
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
};

#define G_SYNC_WRITE_ACCESS_MASK						\
	(VK_ACCESS_2_SHADER_WRITE_BIT |						\
	 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |			\
	 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |	\
	 VK_ACCESS_2_TRANSFER_WRITE_BIT |					\
	 VK_ACCESS_2_HOST_WRITE_BIT |						\
	 VK_ACCESS_2_MEMORY_WRITE_BIT |						\
	 VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)

internal VkMemoryBarrier2 G_SyncMemoryBarrier(const G_AccessSt *src,
											const G_AccessSt *dst);

internal VkImageMemoryBarrier2 G_SyncTextureBarrier(const G_Texture *texture,
												  const G_AccessSt *src,
												  const G_AccessSt *dst,
												  VkImageLayout src_layout,
												  VkImageLayout dst_layout,
												  u32 base_mip,   u32 mip_count,    // VK_REMAINING_MIP_LEVELS
												  u32 base_layer, u32 layer_count); // VK_REMAINING_ARRAY_LAYERS

internal VkBufferMemoryBarrier2 G_SyncBufferBarrier(const G_Buffer *buffer,
												  const G_AccessSt *src,
												  const G_AccessSt *dst,
												  u64 offset, u64 size); // VK_WHOLE_SIZE

#endif // GRAPHICS_SYNC_H
