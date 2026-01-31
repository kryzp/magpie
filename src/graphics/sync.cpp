#include "sync.h"

#include "texture.h"
#include "gpu_buffer.h"

using namespace gfx;

bool sync::texture_access_is_write(TextureAccessType type)
{
	return
		type == TEXTURE_ACCESS_COLOUR_ATTACHMENT ||
		type == TEXTURE_ACCESS_DEPTH_ATTACHMENT ||
		type == TEXTURE_ACCESS_STORAGE_WRITE ||
		type == TEXTURE_ACCESS_BLIT_DST ||
		type == TEXTURE_ACCESS_COPY_DST;
}

TextureAccess sync::get_src_texture_access(TextureAccessType usage)
{
	TextureAccess info = {};

	switch (usage) {
		case TEXTURE_ACCESS_UNDEFINED:
			info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			info.stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;

		case TEXTURE_ACCESS_GENERAL:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_COLOUR_ATTACHMENT:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_DEPTH_ATTACHMENT:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;

		case TEXTURE_ACCESS_SAMPLED:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;

		case TEXTURE_ACCESS_graphics_rw:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_STORAGE_READ:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;

		case TEXTURE_ACCESS_STORAGE_WRITE:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_BLIT_SRC:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;

		case TEXTURE_ACCESS_BLIT_DST:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_COPY_SRC:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;

		case TEXTURE_ACCESS_COPY_DST:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_PRESENT:
			info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
	}

	return info;
}

TextureAccess sync::get_dst_texture_access(TextureAccessType usage)
{
	TextureAccess info = {};

	switch (usage) {
		case TEXTURE_ACCESS_UNDEFINED:
			info.layout = VK_IMAGE_LAYOUT_UNDEFINED;
			info.stage  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;

		case TEXTURE_ACCESS_GENERAL:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_COLOUR_ATTACHMENT:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_DEPTH_ATTACHMENT:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			info.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;

		case TEXTURE_ACCESS_SAMPLED:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT;
			break;

		case TEXTURE_ACCESS_graphics_rw:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_STORAGE_READ:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT;
			break;

		case TEXTURE_ACCESS_STORAGE_WRITE:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_BLIT_SRC:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;

		case TEXTURE_ACCESS_BLIT_DST:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_BLIT_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_COPY_SRC:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;

		case TEXTURE_ACCESS_COPY_DST:
			info.layout = VK_IMAGE_LAYOUT_GENERAL;
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;

		case TEXTURE_ACCESS_PRESENT:
			info.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			info.stage  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;
	}

	return info;
}

GpuBufferAccess sync::get_src_buffer_access(GpuBufferAccessType usage)
{
	GpuBufferAccess info = {};

	switch (usage) {
		case GPU_BUFFER_ACCESS_UNDEFINED:
			info.stage  = VK_PIPELINE_STAGE_2_NONE;
			info.access = VK_ACCESS_2_NONE;
			break;

		case GPU_BUFFER_ACCESS_GRAPHICS_READ_WRITE:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE:
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
			
		case GPU_BUFFER_ACCESS_INDEX:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			info.access = VK_ACCESS_2_INDEX_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_VERTEX:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			info.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_INDIRECT:
			info.stage  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
			info.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_TRANSFER_SRC:
			info.stage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_TRANSFER_DST:
			info.stage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;

		case GPU_BUFFER_ACCESS_COPY_SRC:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_COPY_DST:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
	}

	return info;
}

GpuBufferAccess sync::get_dst_buffer_access(GpuBufferAccessType usage)
{
	GpuBufferAccess info = {};

	switch (usage) {
		case GPU_BUFFER_ACCESS_UNDEFINED:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			info.access = VK_ACCESS_2_NONE;
			break;

		case GPU_BUFFER_ACCESS_GRAPHICS_READ_WRITE:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;

		case GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE:
			info.stage  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			info.access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
			break;
			
		case GPU_BUFFER_ACCESS_INDEX:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			info.access = VK_ACCESS_2_INDEX_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_VERTEX:
			info.stage  = VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			info.access = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_INDIRECT:
			info.stage  = VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
			info.access = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_TRANSFER_SRC:
			info.stage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_TRANSFER_DST:
			info.stage  = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;

		case GPU_BUFFER_ACCESS_COPY_SRC:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_READ_BIT;
			break;

		case GPU_BUFFER_ACCESS_COPY_DST:
			info.stage  = VK_PIPELINE_STAGE_2_COPY_BIT;
			info.access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			break;
	}

	return info;
}

VkImageMemoryBarrier2 sync::texture_memory_barrier(
	const Texture *texture,
	const TextureAccess &src_access_info,
	const TextureAccess &dst_access_info,
	u32 base_mip, u32 mip_count,
	u32 base_layer, u32 layer_count
)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.image = texture->get_handle();

	barrier.subresourceRange.baseMipLevel = base_mip;
	barrier.subresourceRange.levelCount = mip_count;
	barrier.subresourceRange.baseArrayLayer = base_layer;
	barrier.subresourceRange.layerCount = layer_count;
	barrier.subresourceRange.aspectMask = texture->get_aspects();

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
	const GpuBuffer *buffer,
	const GpuBufferAccess &src_access_info,
	const GpuBufferAccess &dst_access_info
)
{
	VkBufferMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

	barrier.buffer = buffer->get_handle();
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
