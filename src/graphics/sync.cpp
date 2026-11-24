#include "graphics/sync.h"

VkMemoryBarrier2 gfx::sync::MemoryBarrier(
	const AccessState &src,
	const AccessState &dst
)
{
	VkMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;

	barrier.srcAccessMask = src.access;
	barrier.dstAccessMask = dst.access;

	barrier.srcStageMask = src.stage;
	barrier.dstStageMask = dst.stage;

	return barrier;
}

VkImageMemoryBarrier2 gfx::sync::TextureBarrier(
	const Texture *texture,
	const AccessState &src,
	const AccessState &dst,
	VkImageLayout src_layout,
	VkImageLayout dst_layout,
	unsigned base_mip, unsigned mip_count,
	unsigned base_layer, unsigned layer_count
)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = texture->vk_handle;

	barrier.subresourceRange.aspectMask = texture->aspect_flags;
	barrier.subresourceRange.baseMipLevel = base_mip;
	barrier.subresourceRange.levelCount = mip_count;
	barrier.subresourceRange.baseArrayLayer = base_layer;
	barrier.subresourceRange.layerCount = layer_count;

	barrier.oldLayout = src_layout;
	barrier.newLayout = dst_layout;

	barrier.srcAccessMask = src.access;
	barrier.dstAccessMask = dst.access;

	barrier.srcStageMask = src.stage;
	barrier.dstStageMask = dst.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	return barrier;
}

VkBufferMemoryBarrier2 gfx::sync::BufferBarrier(
	const Buffer *buffer,
	const AccessState &src,
	const AccessState &dst,
	u64 offset, u64 size
)
{
	VkBufferMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

	barrier.buffer = buffer->vk_handle;
	barrier.offset = offset;
	barrier.size = size;

	barrier.srcAccessMask = src.access;
	barrier.dstAccessMask = dst.access;

	barrier.srcStageMask = src.stage;
	barrier.dstStageMask = dst.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	return barrier;
}
