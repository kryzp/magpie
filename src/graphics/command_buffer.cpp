#include "command_buffer.h"
#include "device.h"
#include "vk_check.h"

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
	rendering_info.renderArea.offset = { 0, 0 };
	rendering_info.renderArea.extent = { info.width, info.height };
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
	scissor.offset = { 0, 0 };
	scissor.extent = { info.width, info.height };

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
	VkShaderStageFlags stage_flags,
	VkPipelineLayout layout, u32 first,
	const Vector<VkDescriptorSet> &descriptors,
	const Vector<u32> &dynamic_offsets
)
{
	VkBindDescriptorSetsInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = layout;
	info.firstSet = first;
	info.descriptorSetCount = descriptors.size();
	info.pDescriptorSets = descriptors.data();
	info.dynamicOffsetCount = dynamic_offsets.size();
	info.pDynamicOffsets = dynamic_offsets.data();

	vkCmdBindDescriptorSets2(handle, &info);
}

void CommandBuffer::bind_bindless(
	VkShaderStageFlags stage_flags,
	VkPipelineLayout layout,
	const BindlessResources &bindless
)
{
	VkBindDescriptorSetsInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = layout;
	info.firstSet = 0;
	info.descriptorSetCount = BINDLESS_SET_MAX_ENUM;
	info.pDescriptorSets = bindless.get_sets();
	info.dynamicOffsetCount = 0;
	info.pDynamicOffsets = nullptr;

	vkCmdBindDescriptorSets2(handle, &info);
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

void CommandBuffer::bind_index_buffer(
	const GpuBuffer *buffer,
	u64 offset
)
{
	vkCmdBindIndexBuffer2(
		handle,
		buffer->get_handle(),
		offset,
		VK_WHOLE_SIZE,
		VK_INDEX_TYPE_UINT32
	);
}

void CommandBuffer::push_constants(
	VkPipelineLayout layout,
	VkShaderStageFlags stage_flags,
	u64 size, const void *data,
	u32 offset
)
{
	VkPushConstantsInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
	info.layout = layout;
	info.stageFlags = stage_flags;
	info.offset = offset;
	info.size = size;
	info.pValues = data;

	vkCmdPushConstants2(handle, &info);
}

void CommandBuffer::set_line_width(float thickness)
{
	vkCmdSetLineWidth(handle, thickness);
}

void CommandBuffer::draw(
	u32 vertex_count,
	u32 instance_count,
	u32 first_vertex,
	u32 first_instance
)
{
	vkCmdDraw(handle, vertex_count, instance_count, first_vertex, first_instance);
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
	const GpuBuffer *buffer, u64 offset,
	u32 count, u32 stride
)
{
	vkCmdDrawIndexedIndirect(
		handle,
		buffer->get_handle(), offset,
		count, stride
	);
}

void CommandBuffer::draw_indexed_indirect_count(
	const GpuBuffer *buffer, u64 offset,
	const GpuBuffer *count_buffer, u64 count_offset,
	u32 max_count, u32 stride
)
{
	vkCmdDrawIndexedIndirectCount(
		handle,
		buffer->get_handle(), offset,
		count_buffer->get_handle(), count_offset,
		max_count, stride
	);
}

void CommandBuffer::draw_mesh_tasks_indirect_count(
	const GpuBuffer *buffer, u64 offset,
	const GpuBuffer *count_buffer, u64 count_offset,
	u32 max_count, u32 stride
)
{
	vkCmdDrawMeshTasksIndirectCountEXT(
		handle,
		buffer->get_handle(), offset,
		count_buffer->get_handle(), count_offset,
		max_count, stride
	);
}

void CommandBuffer::dispatch(u32 x, u32 y, u32 z)
{
	vkCmdDispatch(handle, x, y, z);
}

void CommandBuffer::dispatch_indirect(const GpuBuffer *buffer, u64 offset)
{
	vkCmdDispatchIndirect(handle, buffer->get_handle(), offset);
}

void CommandBuffer::blit(
	const Texture *src,
	const Texture *dst,
	const Vector<VkImageBlit2> &regions,
	VkFilter filter
)
{
	VkBlitImageInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
	info.srcImage = src->get_handle();
	info.dstImage = dst->get_handle();
	info.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.regionCount = regions.size();
	info.pRegions = regions.data();
	info.filter = filter;

	vkCmdBlitImage2(handle, &info);
}

// Transitions the texture to TEXTURE_ACCESS_SAMPLED.
void CommandBuffer::generate_mipmaps(const Texture *texture)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = texture->get_handle();
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = texture->get_aspects();
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = texture->get_layer_count();
	barrier.subresourceRange.levelCount = 1;

	for (int i = 1; i < texture->get_mipmap_count(); i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

		this->pipeline_barrier(0, {}, {}, { barrier });

		for (int face = 0; face < texture->get_layer_count(); face++) {
			int src_mip_width  = (int)texture->get_width()  >> (i - 1);
			int src_mip_height = (int)texture->get_height() >> (i - 1);
			int dst_mip_width  = (int)texture->get_width()  >> (i - 0);
			int dst_mip_height = (int)texture->get_height() >> (i - 0);

			VkImageBlit2 blit = {};
			blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { src_mip_width, src_mip_height, 1 };

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { dst_mip_width, dst_mip_height, 1 };

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
	}

	barrier.subresourceRange.baseMipLevel = texture->get_mipmap_count() - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

	this->pipeline_barrier(0, {}, {}, { barrier });
}

void CommandBuffer::copy_buffer_to_buffer(
	const GpuBuffer *src,
	const GpuBuffer *dst,
	const Vector<VkBufferCopy2> &regions
)
{
	VkCopyBufferInfo2 copy_info = {};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
	copy_info.srcBuffer = src->get_handle();
	copy_info.dstBuffer = dst->get_handle();
	copy_info.regionCount = regions.size();
	copy_info.pRegions = regions.data();

	vkCmdCopyBuffer2(handle, &copy_info);
}

void CommandBuffer::copy_buffer_to_texture(
	const GpuBuffer *src,
	const Texture *dst,
	u64 buffer_offset
)
{
	VkBufferImageCopy2 region = {};
	region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
	region.bufferOffset = buffer_offset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { dst->get_width(), dst->get_height(), 1 };

	copy_buffer_to_texture_region(src, dst, { region });
}

void CommandBuffer::copy_buffer_to_texture_region(
	const GpuBuffer *src,
	const Texture *dst,
	const Vector<VkBufferImageCopy2> &regions
)
{
	VkCopyBufferToImageInfo2 copy_info = {};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.srcBuffer = src->get_handle();
	copy_info.dstImage = dst->get_handle();
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	copy_info.regionCount = regions.size();
	copy_info.pRegions = regions.data();

	vkCmdCopyBufferToImage2(handle, &copy_info);
}

void CommandBuffer::fill_buffer(
	const GpuBuffer *buffer,
	u64 offset, u64 size, u32 data
)
{
	vkCmdFillBuffer(
		handle,
		buffer->get_handle(),
		offset, size, data
	);
}

void CommandBuffer::begin_query(VkQueryPool pool, u32 query, VkQueryControlFlags flags)
{
	vkCmdBeginQuery(handle, pool, query, flags);
}

void CommandBuffer::end_query(VkQueryPool pool, u32 query)
{
	vkCmdEndQuery(handle, pool, query);
}

void CommandBuffer::reset_queries(VkQueryPool pool, u32 first, u32 count)
{
	vkCmdResetQueryPool(handle, pool, first, count);
}

void CommandBuffer::write_timestamp(VkPipelineStageFlags2 stage, VkQueryPool pool, u32 index)
{
	vkCmdWriteTimestamp2(handle, stage, pool, index);
}
