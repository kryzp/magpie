#include "sync.h"

struct gfx_texture_access gfx_sync_get_src_texture_access(enum gfx_texture_access_type type)
{
	struct gfx_texture_access info = {0};
	
	switch (type) {
	case GFX_TEXTURE_ACCESS_TYPE_undefined:
		info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.stage  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_general:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_graphics_r:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_graphics_rw:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_compute_r:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_compute_rw:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_colour:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		info.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_depth:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_blit_src:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_blit_dst:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_copy_src:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_copy_dst:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_present:
		info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	}

	return info;
}

struct gfx_texture_access gfx_sync_get_dst_texture_access(enum gfx_texture_access_type type)
{
	struct gfx_texture_access info = {0};
	
	switch (type) {
	case GFX_TEXTURE_ACCESS_TYPE_undefined:
		info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_general:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_graphics_r:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_graphics_rw:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_compute_r:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_compute_rw:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_colour:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		info.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_depth:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_blit_src:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
		info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_blit_dst:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_copy_src:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_copy_dst:
		info.layout = VK_IMAGE_LAYOUT_GENERAL;
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case GFX_TEXTURE_ACCESS_TYPE_present:
		info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	}

	return info;
}

struct gfx_buffer_access gfx_sync_get_src_buffer_access(enum gfx_buffer_access_type type)
{
	struct gfx_buffer_access info = {0};
	
	switch (type) {
	case GFX_BUFFER_ACCESS_TYPE_undefined:
		info.stage  = VK_PIPELINE_STAGE_2_NONE;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GFX_BUFFER_ACCESS_TYPE_graphics_rw:
		info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_compute_rw:
		info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_copy_src:
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_copy_dst:
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_indirect:
		info.stage  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		info.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		break;
	}

	return info;
}

struct gfx_buffer_access gfx_sync_get_dst_buffer_access(enum gfx_buffer_access_type type)
{
	struct gfx_buffer_access info = {0};
	
	switch (type) {
	case GFX_BUFFER_ACCESS_TYPE_undefined:
		info.stage  = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		info.access = VK_ACCESS_2_NONE;
		break;
	case GFX_BUFFER_ACCESS_TYPE_graphics_rw:
		info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_compute_rw:
		info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_copy_src:
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_copy_dst:
		info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
		info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		break;
	case GFX_BUFFER_ACCESS_TYPE_indirect:
		info.stage  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		info.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		break;
	}

	return info;
}

VkImageMemoryBarrier2 gfx_sync_texture_memory_barrier(struct gfx_texture *texture,
						      struct gfx_texture_access src_access_info,
						      struct gfx_texture_access dst_access_info,
						      u32 base_level, u32 level_count,
						      u32 base_layer, u32 layer_count)
{	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = texture->handle;

	barrier.oldLayout = src_access_info.layout;
	barrier.newLayout = dst_access_info.layout;

	barrier.srcAccessMask = src_access_info.access;
	barrier.dstAccessMask = dst_access_info.access;

	barrier.srcStageMask = src_access_info.stage;
	barrier.dstStageMask = dst_access_info.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.subresourceRange.baseMipLevel = base_level;
	barrier.subresourceRange.levelCount = level_count;
	barrier.subresourceRange.baseArrayLayer = base_layer;
	barrier.subresourceRange.layerCount = layer_count;
	barrier.subresourceRange.aspectMask = texture->aspect_flags;

	return barrier;
}

VkBufferMemoryBarrier2 gfx_sync_buffer_memory_barrier(struct gfx_buffer *buffer,
						      struct gfx_buffer_access src_access_info,
						      struct gfx_buffer_access dst_access_info)
{	VkBufferMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

	barrier.srcAccessMask = src_access_info.access;
	barrier.dstAccessMask = dst_access_info.access;

	barrier.srcStageMask = src_access_info.stage;
	barrier.dstStageMask = dst_access_info.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	
	barrier.buffer = buffer->handle;
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;
	
	return barrier;
}
