
internal b32
GFX_SyncIsWriteAccess(VkAccessFlags2 access_flags)
{
	static const VkAccessFlags2 write_bits =
		VK_ACCESS_2_SHADER_WRITE_BIT |
		VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
		VK_ACCESS_2_TRANSFER_WRITE_BIT |
		VK_ACCESS_2_HOST_WRITE_BIT |
		VK_ACCESS_2_MEMORY_WRITE_BIT |
		VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

	return (access_flags & write_bits) != 0;
}

internal VkImageMemoryBarrier2
GFX_SyncTextureBarrier(const GFX_Texture *texture,
					   const GFX_AccessSt *src,
					   const GFX_AccessSt *dst,
					   VkImageLayout src_layout,
					   VkImageLayout dst_layout,
					   u32 base_mip,   u32 mip_count,
					   u32 base_layer, u32 layer_count)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = texture->handle;
	
	barrier.subresourceRange.aspectMask = texture->aspect_flags;
	barrier.subresourceRange.baseMipLevel = base_mip;
	barrier.subresourceRange.levelCount = mip_count;
	barrier.subresourceRange.baseArrayLayer = base_layer;
	barrier.subresourceRange.layerCount = layer_count;

	barrier.oldLayout = src_layout;
	barrier.newLayout = dst_layout;

	barrier.srcAccessMask = src->access;
	barrier.dstAccessMask = dst->access;

	barrier.srcStageMask = src->stage;
	barrier.dstStageMask = dst->stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	return barrier;
}

internal VkBufferMemoryBarrier2
GFX_SyncBufferBarrier(const GFX_Buffer *buffer,
					  const GFX_AccessSt *src,
					  const GFX_AccessSt *dst,
					  u64 offset, u64 size)
{
	VkBufferMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

	barrier.buffer = buffer->handle;
	barrier.offset = offset;
	barrier.size   = size;

	barrier.srcAccessMask = src->access;
	barrier.dstAccessMask = dst->access;

	barrier.srcStageMask = src->stage;
	barrier.dstStageMask = dst->stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	return barrier;
}
