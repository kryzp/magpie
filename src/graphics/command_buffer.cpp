#include "command_buffer.h"
#include "device.h"

using namespace gfx;

CommandBuffer::CommandBuffer(VkCommandBuffer handle)
	: handle(handle)
{
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::begin()
{
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	GFX_VK_CHECK(
		vkBeginCommandBuffer(handle, &begin_info),
		"Failed to begin recording instant command buffer."
	);
}

void CommandBuffer::end()
{
	GFX_VK_CHECK(
		vkEndCommandBuffer(handle),
		"Failed to record command buffer."
	);
}

void CommandBuffer::begin_rendering(const RenderInfo &info)
{
	VkRenderingInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	rendering_info.renderArea.offset = (VkOffset2D){ 0, 0 };
	rendering_info.renderArea.extent = (VkExtent2D){ info.width, info.height };
	rendering_info.layerCount = 1;
	rendering_info.viewMask = info.view_mask;
	rendering_info.colorAttachmentCount = info.colour_attachments.size();
	rendering_info.pColorAttachments = info.colour_attachments.data();
	rendering_info.pDepthAttachment = info.depth_attachment.imageView ? &info.depth_attachment : nullptr;
	rendering_info.pStencilAttachment = nullptr;

	vkCmdBeginRendering(handle, &rendering_info);

	VkViewport viewport = {};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = (float)info.width;
	viewport.height = (float)info.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor = {};
	scissor.offset = (VkOffset2D){ 0, 0 };
	scissor.extent = (VkExtent2D){ info.width, info.height };

	// We have to update the dynamic states
	// every time we draw something.
	// TODO: Just reset an internal viewport and scissor variable
	//       and only *actually* set it at end_rendering()
	set_viewport(viewport);
	set_scissor(scissor);
}

void CommandBuffer::end_rendering()
{
	vkCmdEndRendering(handle);
}

void CommandBuffer::set_viewport(VkViewport viewport)
{
	// Vulkan uses a Y+ down coordinate system, but
	// we use Y+ up, so we flip the viewport internally
	// to account for this.

	VkViewport corrected_viewport = {};
	corrected_viewport.x = viewport.x;
	corrected_viewport.y = viewport.height + viewport.y;
	corrected_viewport.width = viewport.width;
	corrected_viewport.height = -viewport.height;
	corrected_viewport.minDepth = viewport.minDepth;
	corrected_viewport.maxDepth = viewport.maxDepth;

	vkCmdSetViewport(handle, 0, 1, &corrected_viewport);
}

void CommandBuffer::set_scissor(VkRect2D scissor)
{
	vkCmdSetScissor(handle, 0, 1, &scissor);
}

void CommandBuffer::pipeline_barrier(
	VkDependencyFlags dependency_flags,
	const Vector<VkMemoryBarrier2> &memory_barriers,
	const Vector<VkBufferMemoryBarrier2> &buffer_memory_barriers,
	const Vector<VkImageMemoryBarrier2> &image_memory_barriers
)
{
	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.dependencyFlags = dependency_flags;

	dependency.memoryBarrierCount = memory_barriers.size();
	dependency.pMemoryBarriers = memory_barriers.data();

	dependency.bufferMemoryBarrierCount = buffer_memory_barriers.size();
	dependency.pBufferMemoryBarriers = buffer_memory_barriers.data();

	dependency.imageMemoryBarrierCount = image_memory_barriers.size();
	dependency.pImageMemoryBarriers = image_memory_barriers.data();

	vkCmdPipelineBarrier2(handle, &dependency);
}

void CommandBuffer::bind_descriptors(
	VkPipelineBindPoint bind_point,
	VkPipelineLayout layout, u32 first,
	const Vector<VkDescriptorSet> &descriptors,
	const Vector<u32> &dynamic_offsets
)
{
	vkCmdBindDescriptorSets(
		handle,
		bind_point,
		layout, first,
		descriptors.size(),
		descriptors.data(),
		dynamic_offsets.size(),
		dynamic_offsets.data()
	);
}

void CommandBuffer::bind_bindless(
	VkPipelineBindPoint bind_point,
	VkPipelineLayout layout,
	const BindlessResources &bindless
)
{
	vkCmdBindDescriptorSets(
		handle,
		bind_point,
		layout, 0,
		BINDLESS_SET_MAX_ENUM,
		bindless.get_sets(),
		0, nullptr
	);
}

void CommandBuffer::bind_pipeline(
	VkPipelineBindPoint bind_point,
	VkPipeline pipeline
)
{
	vkCmdBindPipeline(
		handle,
		bind_point,
		pipeline
	);
}

void CommandBuffer::bind_index_buffer(const GpuBuffer &buffer, u64 offset)
{
	vkCmdBindIndexBuffer(
		handle,
		buffer.get_handle(),
		offset,
		VK_INDEX_TYPE_UINT16
	);
}

void CommandBuffer::push_constants(
	VkPipelineLayout layout,
	VkShaderStageFlags stage_flags,
	u64 size, void *data,
	u32 offset
)
{
	vkCmdPushConstants(
		handle,
		layout,
		stage_flags,
		offset,
		size, data
	);
}

void CommandBuffer::draw_vertices_n(u64 count)
{
	vkCmdDraw(handle, count, 1, 0, 0);
}

void CommandBuffer::draw_indexed(
	u32 index_count,
	u32 instance_count,
	u32 first_index,
	s32 vertex_offset,
	u32 first_instance
)
{
	vkCmdDrawIndexed(
		handle,
		index_count,
		instance_count,
		first_index,
		vertex_offset,
		first_instance
	);
}

void CommandBuffer::draw_indexed_indirect(
	const GpuBuffer &buffer,
	u64 offset,
	u32 count,
	u32 stride
)
{
	vkCmdDrawIndexedIndirect(
		handle,
		buffer.get_handle(),
		offset,
		count,
		stride
	);
}

void CommandBuffer::blit(
	const Texture &src,
	const Texture &dst,
	const Vector<VkImageBlit2> &regions,
	VkFilter filter
)
{
	VkBlitImageInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
	info.srcImage = src.get_handle();
	info.dstImage = dst.get_handle();
	info.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.regionCount = regions.size();
	info.pRegions = regions.data();
	info.filter = filter;

	vkCmdBlitImage2(handle, &info);
}

// Transitions the texture to SHADER_READ_ONLY.
void CommandBuffer::generate_mipmaps(Texture &texture)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = texture.get_handle();
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = texture.get_aspects();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = texture.get_layer_count();
	barrier.subresourceRange.levelCount = 1;

	for (int i = 1; i < texture.get_mipmap_count(); i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

		this->pipeline_barrier(0, {}, {}, { barrier });

		for (int face = 0; face < texture.get_layer_count(); face++) {
			int src_mip_width  = (int)texture.get_width()  >> (i - 1);
			int src_mip_height = (int)texture.get_height() >> (i - 1);
			int dst_mip_width  = (int)texture.get_width()  >> (i - 0);
			int dst_mip_height = (int)texture.get_height() >> (i - 0);

			VkImageBlit2 blit = {};
			blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

			blit.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
			blit.srcOffsets[1] = (VkOffset3D){ src_mip_width, src_mip_height, 1 };

			blit.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
			blit.dstOffsets[1] = (VkOffset3D){ dst_mip_width, dst_mip_height, 1 };

			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;

			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;

			this->blit(texture, texture, { blit }, VK_FILTER_LINEAR);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		this->pipeline_barrier(0, {}, {}, { barrier });

		for (u32 k = 0; k < texture.get_layer_count(); k++)
			texture.set_access(i, k, 0, sync::TEXTURE_ACCESS_graphics_r);
	}

	barrier.subresourceRange.baseMipLevel = texture.get_mipmap_count() - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

	this->pipeline_barrier(0, {}, {}, { barrier });

	for (u32 k = 0; k < texture.get_layer_count(); k++)
		texture.set_access(0, k, 0, sync::TEXTURE_ACCESS_graphics_r);
}

void CommandBuffer::copy_buffer_to_buffer(
	const GpuBuffer &src,
	const GpuBuffer &dst,
	const Vector<VkBufferCopy> &regions
)
{
	vkCmdCopyBuffer(
		handle,
		src.get_handle(),
		dst.get_handle(),
		regions.size(),
		regions.data()
	);
}

void CommandBuffer::copy_buffer_to_texture(
	const GpuBuffer &src,
	const Texture &dst
)
{
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = (VkOffset3D){ 0, 0, 0 };
	region.imageExtent = (VkExtent3D){ dst.get_width(), dst.get_height(), 1 };

	copy_buffer_to_texture_regions(src, dst, { region });
}

void CommandBuffer::copy_buffer_to_texture_regions(
	const GpuBuffer &src,
	const Texture &dst,
	const Vector<VkBufferImageCopy> &regions
)
{
	vkCmdCopyBufferToImage(
		handle,
		src.get_handle(),
		dst.get_handle(),
		VK_IMAGE_LAYOUT_GENERAL,
		regions.size(), regions.data()
	);
}

void CommandBuffer::dispatch(u32 x, u32 y, u32 z)
{
	vkCmdDispatch(handle, x, y, z);
}
