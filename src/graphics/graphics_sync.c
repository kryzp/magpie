
static VkMemoryBarrier2 G_SyncMemoryBarrier(const G_AccessSt *src,
											const G_AccessSt *dst)
{
	VkMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;

	barrier.srcAccessMask = src->access;
	barrier.dstAccessMask = dst->access;

	barrier.srcStageMask = src->stage;
	barrier.dstStageMask = dst->stage;

	return barrier;
}

static VkImageMemoryBarrier2 G_SyncTextureBarrier(const G_Texture *texture,
												  const G_AccessSt *src,
												  const G_AccessSt *dst,
												  VkImageLayout src_layout,
												  VkImageLayout dst_layout,
												  u32 base_mip,   u32 mip_count,
												  u32 base_layer, u32 layer_count)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = texture->vk_handle;
	
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

static VkBufferMemoryBarrier2 G_SyncBufferBarrier(const G_Buffer *buffer,
												  const G_AccessSt *src,
												  const G_AccessSt *dst,
												  u64 offset, u64 size)
{
	VkBufferMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

	barrier.buffer = buffer->vk_handle;
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
