#include "sync.h"

#include "texture.h"
#include "gpu_buffer.h"

using namespace gfx;

sync::TextureAccess sync::get_src_texture_access(TextureAccessType type)
{
	TextureAccess info = {};

	switch (type) {
		case TEXTURE_ACCESS_undefined:
			info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			info.stage  = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
		case TEXTURE_ACCESS_general:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_graphics_r:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
		case TEXTURE_ACCESS_graphics_rw:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_compute_r:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
		case TEXTURE_ACCESS_compute_rw:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_colour:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_depth:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_blit_src:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
		case TEXTURE_ACCESS_blit_dst:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_copy_src:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;
		case TEXTURE_ACCESS_copy_dst:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_present:
			info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
	}

	return info;
}

sync::TextureAccess sync::get_dst_texture_access(TextureAccessType type)
{
	TextureAccess info = {};

	switch (type) {
		case TEXTURE_ACCESS_undefined:
			info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			info.stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
		case TEXTURE_ACCESS_general:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_graphics_r:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT;
			break;
		case TEXTURE_ACCESS_graphics_rw:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_compute_r:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT;
			break;
		case TEXTURE_ACCESS_compute_rw:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_colour:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_depth:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
			info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_blit_src:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;
		case TEXTURE_ACCESS_blit_dst:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_copy_src:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;
		case TEXTURE_ACCESS_copy_dst:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case TEXTURE_ACCESS_present:
			info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
	}

	return info;
}

sync::GpuBufferAccess sync::get_src_buffer_access(GpuBufferAccessType type)
{
	GpuBufferAccess info = {};

	switch (type) {
		case GPU_BUFFER_ACCESS_undefined:
			info.stage  = VK_PIPELINE_STAGE_2_NONE;
			info.access = VK_ACCESS_2_NONE;
			break;
		case GPU_BUFFER_ACCESS_graphics_rw:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case GPU_BUFFER_ACCESS_compute_rw:
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case GPU_BUFFER_ACCESS_copy_src:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;
		case GPU_BUFFER_ACCESS_copy_dst:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case GPU_BUFFER_ACCESS_indirect:
			info.stage  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
			info.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
			break;
	}

	return info;
}

sync::GpuBufferAccess sync::get_dst_buffer_access(GpuBufferAccessType type)
{
	GpuBufferAccess info = {};

	switch (type) {
		case GPU_BUFFER_ACCESS_undefined:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
		case GPU_BUFFER_ACCESS_graphics_rw:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case GPU_BUFFER_ACCESS_compute_rw:
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
		case GPU_BUFFER_ACCESS_copy_src:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;
		case GPU_BUFFER_ACCESS_copy_dst:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
		case GPU_BUFFER_ACCESS_indirect:
			info.stage  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
			info.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
			break;
	}

	return info;
}

VkImageMemoryBarrier2 sync::texture_memory_barrier(
	const Texture &texture,
	const TextureAccess &src_access_info,
	const TextureAccess &dst_access_info,
	u32 base_mip, u32 mip_count,
	u32 base_layer, u32 layer_count
)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = texture.get_handle();

	barrier.subresourceRange.baseMipLevel = base_mip;
	barrier.subresourceRange.levelCount = mip_count;
	barrier.subresourceRange.baseArrayLayer = base_layer;
	barrier.subresourceRange.layerCount = layer_count;
	barrier.subresourceRange.aspectMask = texture.get_aspects();

	barrier.oldLayout = src_access_info.layout;
	barrier.newLayout = dst_access_info.layout;

	barrier.srcAccessMask = src_access_info.access;
	barrier.dstAccessMask = dst_access_info.access;

	barrier.srcStageMask = src_access_info.stage;
	barrier.dstStageMask = dst_access_info.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	return barrier;
}

VkBufferMemoryBarrier2 sync::buffer_memory_barrier(
	const GpuBuffer &buffer,
	const GpuBufferAccess &src_access_info,
	const GpuBufferAccess &dst_access_info
)
{
	VkBufferMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

	barrier.buffer = buffer.get_handle();
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;

	barrier.srcAccessMask = src_access_info.access;
	barrier.dstAccessMask = dst_access_info.access;

	barrier.srcStageMask = src_access_info.stage;
	barrier.dstStageMask = dst_access_info.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	return barrier;
}
