
internal void CmdBegin(CommandBuffer *cmd)
{
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd->handle, &begin_info),
		 "Failed to begin recording instant command buffer.");
}

internal void CmdEnd(CommandBuffer *cmd)
{
	VK_CHECK(vkEndCommandBuffer(cmd->handle),
		 "Failed to record command buffer.");
}

internal void CmdSetViewport(CommandBuffer *cmd, VkViewport viewport)
{
	// Vulkan uses a Y+ down coordinate system but
	// we use Y+ up, so we flip the viewport internally
	// to account for this.

	VkViewport corrected_viewport = {0};
	corrected_viewport.x = viewport.x;
	corrected_viewport.y = viewport.height + viewport.y;
	corrected_viewport.width = viewport.width;
	corrected_viewport.height = -viewport.height;
	corrected_viewport.minDepth = viewport.minDepth;
	corrected_viewport.maxDepth = viewport.maxDepth;

	vkCmdSetViewport(cmd->handle, 0, 1, &corrected_viewport);
}

internal void CmdSetScissor(CommandBuffer *cmd, VkRect2D scissor)
{
	vkCmdSetScissor(cmd->handle, 0, 1, &scissor);
}

internal void CmdBeginRendering(CommandBuffer *cmd, RenderInfo *info)
{
	VkRenderingInfo rendering_info = {0};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	rendering_info.renderArea.offset = (VkOffset2D){ 0, 0 };
	rendering_info.renderArea.extent = (VkExtent2D){ info->width, info->height };
	rendering_info.layerCount = 1;
	rendering_info.viewMask = info->view_mask;
	rendering_info.colorAttachmentCount = info->colour_attachment_count;
	rendering_info.pColorAttachments = info->colour_attachments;
	rendering_info.pDepthAttachment = info->depth_attachment.imageView ? &info->depth_attachment : 0;
	rendering_info.pStencilAttachment = 0;

	vkCmdBeginRendering(cmd->handle, &rendering_info);

	VkViewport viewport = {0};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = (f32)info->width;
	viewport.height = (f32)info->height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D){ 0, 0 };
	scissor.extent = (VkExtent2D){ info->width, info->height };

	// We have to update the dynamic states
	// every time we draw something.
	CmdSetViewport(cmd, viewport);
	CmdSetScissor(cmd, scissor);
}

internal void CmdEndRendering(CommandBuffer *cmd)
{
	vkCmdEndRendering(cmd->handle);
}

internal void CmdPipelineBarrier(CommandBuffer *cmd,
				 VkDependencyFlags dependency_flags,
				 u32 memory_barrier_count,        VkMemoryBarrier2       *memory_barriers,
				 u32 buffer_memory_barrier_count, VkBufferMemoryBarrier2 *buffer_memory_barriers,
				 u32 image_memory_barrier_count,  VkImageMemoryBarrier2  *image_memory_barriers)
{
	VkDependencyInfo dependency = {0};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.dependencyFlags = dependency_flags;

	dependency.memoryBarrierCount = memory_barrier_count;
	dependency.pMemoryBarriers = memory_barriers;

	dependency.bufferMemoryBarrierCount = buffer_memory_barrier_count;
	dependency.pBufferMemoryBarriers = buffer_memory_barriers;

	dependency.imageMemoryBarrierCount = image_memory_barrier_count;
	dependency.pImageMemoryBarriers = image_memory_barriers;

	vkCmdPipelineBarrier2(cmd->handle, &dependency);
}

internal void CmdBindDescriptors(CommandBuffer *cmd,
				 VkPipelineBindPoint bind_point,
				 VkPipelineLayout layout, u32 first,
				 u32 descriptor_count,
				 VkDescriptorSet *descriptors,
				 u32 dynamic_offset_count, u32 *dynamic_offsets)
{
	vkCmdBindDescriptorSets(cmd->handle,
				bind_point,
				layout, first,
				descriptor_count,
				descriptors,
				dynamic_offset_count,
				dynamic_offsets);
}

internal void CmdBindBindless(CommandBuffer *cmd,
			      VkPipelineBindPoint bind_point,
			      VkPipelineLayout layout)
{
	CmdBindDescriptors(cmd,
			   bind_point,
			   layout,
			   0,
			   BindlessSetBinding_MaxEnum, graphics_device->bindless.sets,
			   0, NULL);
}

internal void CmdBindPipeline(CommandBuffer *cmd,
			      VkPipelineBindPoint bind_point,
			      VkPipeline pipeline)
{
	vkCmdBindPipeline(cmd->handle,
			  bind_point,
			  pipeline);
}

internal void CmdBindVertexBuffer(CommandBuffer *cmd,
				  u32 binding,
				  GPUBuffer *buffer,
				  u64 offset)
{
	vkCmdBindVertexBuffers(cmd->handle,
			       binding,
			       1, &buffer->handle,
			       &offset);
}

internal void CmdBindIndexBuffer(CommandBuffer *cmd,
				 GPUBuffer *buffer,
				 u64 offset)
{
	vkCmdBindIndexBuffer(cmd->handle,
			     buffer->handle,
			     offset,
			     VK_INDEX_TYPE_UINT16);
}

internal void CmdPushConstants(CommandBuffer *cmd,
			       VkPipelineLayout layout,
			       VkShaderStageFlags stage_flags,
			       u32 size, void *data)
{
	vkCmdPushConstants(cmd->handle,
			   layout,
			   stage_flags,
			   0,
			   size, data);
}

internal void CmdPushConstantsOffset(CommandBuffer *cmd,
				     VkPipelineLayout layout,
				     VkShaderStageFlags stage_flags,
				     u32 size, void *data, u32 offset)
{
	vkCmdPushConstants(cmd->handle,
			   layout,
			   stage_flags,
			   offset,
			   size, data);
}
	
internal void CmdDrawVerticesN(CommandBuffer *cmd, u32 vertex_count)
{
	vkCmdDraw(cmd->handle, vertex_count, 1, 0, 0);
}

internal void CmdDrawIndexed(CommandBuffer *cmd,
			     u32 index_count,
			     u32 instance_count,
			     u32 first_index,
			     i32 vertex_offset,
			     u32 first_instance)
{
	vkCmdDrawIndexed(cmd->handle,
			 index_count,
			 instance_count,
			 first_index,
			 vertex_offset,
			 first_instance);
}

internal void CmdDrawIndexedIndirect(CommandBuffer *cmd,
				     GPUBuffer *buffer,
				     u64 offset,
				     u32 count,
				     u32 stride)
{
	vkCmdDrawIndexedIndirect(cmd->handle,
				 buffer->handle,
				 offset,
				 count,
				 stride);
}

internal void CmdBlitImage(CommandBuffer *cmd,
			   Image *src, VkImageLayout src_layout,
			   Image *dst, VkImageLayout dst_layout,
			   u32 region_count, VkImageBlit *regions,
			   VkFilter filter)
{
	vkCmdBlitImage(cmd->handle,
		       src->image, src_layout,
		       dst->image, dst_layout,
		       region_count, regions,
		       filter);
}

// Input:  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
// Output: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
internal void CmdGenerateMipmaps(CommandBuffer *cmd, Image *image)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = image->image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = ImageLayerCount(image);
	barrier.subresourceRange.levelCount = 1;

	for (i32 i = 1; i < image->mipmap_count; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

		CmdPipelineBarrier(cmd, 0,
				   0, NULL,
				   0, NULL,
				   1, &barrier);

		for (i32 face = 0; face < ImageFaceCount(image); face++) {
			i32 src_mip_width  = (i32)image->width  >> (i - 1);
			i32 src_mip_height = (i32)image->height >> (i - 1);
			i32 dst_mip_width  = (i32)image->width  >> (i - 0);
			i32 dst_mip_height = (i32)image->height >> (i - 0);

			VkImageBlit blit = {0};

			blit.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
			blit.srcOffsets[1] = (VkOffset3D){ src_mip_width, src_mip_height, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
			blit.dstOffsets[1] = (VkOffset3D){ dst_mip_width, dst_mip_height, 1 };
			
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;

			CmdBlitImage(cmd,
				     image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				     image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				     1, &blit,
				     VK_FILTER_LINEAR);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		CmdPipelineBarrier(cmd, 0,
				   0, NULL,
				   0, NULL,
				   1, &barrier);
	}

	barrier.subresourceRange.baseMipLevel = image->mipmap_count - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

	CmdPipelineBarrier(cmd, 0,
			   0, NULL,
			   0, NULL,
			   1, &barrier);
}

internal void CmdCopyBufferToBuffer(CommandBuffer *cmd,
				    GPUBuffer *src,
				    GPUBuffer *dst,
				    u32 region_count, VkBufferCopy *regions)
{
	vkCmdCopyBuffer(cmd->handle,
			src->handle,
			dst->handle,
			region_count, regions);
}

// Image must be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL.
// TODO: An image could also be in VK_IMAGE_LAYOUT_GENERAL.
internal void CmdCopyBufferToImageMultiRegion(CommandBuffer *cmd,
					      GPUBuffer *buffer,
					      Image *image,
					      u32 region_count, VkBufferImageCopy *regions)
{
	vkCmdCopyBufferToImage(cmd->handle,
			       buffer->handle,
			       image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			       region_count, regions);
}

internal void CmdCopyBufferToImage(CommandBuffer *cmd, GPUBuffer *buffer,
				   Image *image)
{
	VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = (VkOffset3D){ 0, 0, 0 };
	region.imageExtent = (VkExtent3D){ image->width, image->height, 1 };

	CmdCopyBufferToImageMultiRegion(cmd, buffer, image, 1, &region);
}

internal void CmdDispatch(CommandBuffer *cmd, u32 x, u32 y, u32 z)
{
	vkCmdDispatch(cmd->handle, x, y, z);
}
