
internal void
GFX_CmdBegin(const GFX_CmdBuffer *cmd)
{
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	GFX_VK_CHECK(vkBeginCommandBuffer(cmd->handle, &begin_info),
				 "Failed to begin recording instant command buffer.");
}

internal void
GFX_CmdEnd(const GFX_CmdBuffer *cmd)
{
	GFX_VK_CHECK(vkEndCommandBuffer(cmd->handle),
				 "Failed to record command buffer.");
}

internal void
GFX_CmdBeginRendering(const GFX_CmdBuffer *cmd, const GFX_RenderInfo *info)
{
	VkRenderingInfo rendering_info = {0};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	rendering_info.renderArea.offset = (VkOffset2D) { 0, 0 };
	rendering_info.renderArea.extent = (VkExtent2D) { info->width, info->height };
	rendering_info.layerCount = 1;
	rendering_info.viewMask = info->view_mask;
	rendering_info.colorAttachmentCount = info->colour_attachment_count;
	rendering_info.pColorAttachments = info->colour_attachments;
	rendering_info.pDepthAttachment = info->depth_attachment.imageView ? &info->depth_attachment : NULL;
	rendering_info.pStencilAttachment = NULL;

	vkCmdBeginRendering(cmd->handle, &rendering_info);

	VkViewport viewport = {0};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = (f32)info->width;
	viewport.height = (f32)info->height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D) { 0, 0 };
	scissor.extent = (VkExtent2D) { info->width, info->height };

	// We have to update the dynamic states
	// every time we draw something.
	// TODO: Just reset an internal viewport and scissor variable
	//       and only *actually* set it at end_rendering()
	GFX_CmdSetViewport(cmd, viewport);
	GFX_CmdSetScissor(cmd, scissor);
}

internal void
GFX_CmdEndRendering(const GFX_CmdBuffer *cmd)
{
	vkCmdEndRendering(cmd->handle);
}

internal void
GFX_CmdSetViewport(const GFX_CmdBuffer *cmd, VkViewport viewport)
{
	// Vulkan uses a Y+ down coordinate system, but
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

internal void
GFX_CmdSetScissor(const GFX_CmdBuffer *cmd, VkRect2D scissor)
{
	vkCmdSetScissor(cmd->handle, 0, 1, &scissor);
}

internal void
GFX_CmdPipelineBarrier(const GFX_CmdBuffer *cmd, VkDependencyFlags dependency_flags,
					   u32 memory_barrier_count, const VkMemoryBarrier2 *memory_barriers,
					   u32 buffer_barrier_count, const VkBufferMemoryBarrier2 *buffer_barriers,
					   u32 image_barrier_count,  const VkImageMemoryBarrier2 *image_barriers)
{
	VkDependencyInfo dependency = {0};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.dependencyFlags = dependency_flags;
	
	dependency.memoryBarrierCount = memory_barrier_count;
	dependency.pMemoryBarriers = memory_barriers;

	dependency.bufferMemoryBarrierCount = buffer_barrier_count;
	dependency.pBufferMemoryBarriers = buffer_barriers;

	dependency.imageMemoryBarrierCount = image_barrier_count;
	dependency.pImageMemoryBarriers = image_barriers;

	vkCmdPipelineBarrier2(cmd->handle, &dependency);
}

internal void
GFX_CmdBindDescriptors(const GFX_CmdBuffer *cmd,
					   VkShaderStageFlags stage_flags,
					   VkPipelineLayout layout, u32 first,
					   u32 descriptor_count, const VkDescriptorSet *descriptors,
					   u32 dynamic_offset_count, const u32 *dynamic_offsets)
{
	VkBindDescriptorSetsInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = layout;
	info.firstSet = first;
	info.descriptorSetCount = descriptor_count;
	info.pDescriptorSets = descriptors;
	info.dynamicOffsetCount = dynamic_offset_count;
	info.pDynamicOffsets = dynamic_offsets;

	vkCmdBindDescriptorSets2(cmd->handle, &info);
}

internal void
GFX_CmdBindBindless(const GFX_CmdBuffer *cmd,
					VkShaderStageFlags stage_flags,
					VkPipelineLayout layout,
					const GFX_Bindless *bindless)
{
	VkBindDescriptorSetsInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = layout;
	info.firstSet = 0;
	info.descriptorSetCount = GFX_BindlessSetKind_COUNT;
	info.pDescriptorSets = bindless->sets;
	info.dynamicOffsetCount = 0;
	info.pDynamicOffsets = NULL;

	vkCmdBindDescriptorSets2(cmd->handle, &info);
}

internal void
GFX_CmdBindPipeline(const GFX_CmdBuffer *cmd,
					VkPipelineBindPoint bind_point,
					VkPipeline pipeline)
{
	vkCmdBindPipeline(cmd->handle, bind_point, pipeline);
}

// TODO: Take in GFX_BufferRange as input?
internal void
GFX_CmdBindIndexBuffer(const GFX_CmdBuffer *cmd,
					   const GFX_Buffer *buffer,
					   u64 offset)
{
	vkCmdBindIndexBuffer2(cmd->handle,
						  buffer->handle,
						  offset, VK_WHOLE_SIZE,
						  VK_INDEX_TYPE_UINT32);
}

internal void
GFX_CmdPushConstants(const GFX_CmdBuffer *cmd,
					 VkPipelineLayout layout,
					 VkShaderStageFlags stage_flags,
					 u64 size, const void *data,
					 u32 offset)
{
	VkPushConstantsInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
	info.layout = layout;
	info.stageFlags = stage_flags;
	info.offset = offset;
	info.size = size;
	info.pValues = data;

	vkCmdPushConstants2(cmd->handle, &info);
}

internal void
GFX_CmdSetLineWidth(const GFX_CmdBuffer *cmd, float thickness)
{
	vkCmdSetLineWidth(cmd->handle, thickness);
}

internal void
GFX_CmdDraw(const GFX_CmdBuffer *cmd,
			u32 vertex_count,
			u32 instance_count,
			u32 first_vertex,
			u32 first_instance)
{
	vkCmdDraw(cmd->handle,
			  vertex_count,
			  instance_count,
			  first_vertex,
			  first_instance);
}

internal void
GFX_CmdDrawIndexed(const GFX_CmdBuffer *cmd,
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

internal void
GFX_CmdDrawIndexedIndirect(const GFX_CmdBuffer *cmd,
						   const GFX_Buffer *buffer, u64 offset,
						   u32 count, u32 stride)
{
	vkCmdDrawIndexedIndirect(cmd->handle,
							 buffer->handle, offset,
							 count, stride);
}

internal void
GFX_CmdDrawIndexedIndirectCount(const GFX_CmdBuffer *cmd,
								const GFX_Buffer *indirect_buffer, u64 indirect_offset,
								const GFX_Buffer *count_buffer, u64 count_offset,
								u32 max_count, u32 stride)
{
	vkCmdDrawIndexedIndirectCount(cmd->handle,
								  indirect_buffer->handle, indirect_offset,
								  count_buffer->handle, count_offset,
								  max_count, stride);
}

internal void
GFX_CmdDrawMeshTasksIndirectCount(const GFX_CmdBuffer *cmd,
								  const GFX_Buffer *indirect_buffer, u64 indirect_offset,
								  const GFX_Buffer *count_buffer, u64 count_offset,
								  u32 max_count, u32 stride)
{
	vkCmdDrawMeshTasksIndirectCountEXT(cmd->handle,
									   indirect_buffer->handle, indirect_offset,
									   count_buffer->handle, count_offset,
									   max_count, stride);
}

internal void
GFX_CmdDispatch(const GFX_CmdBuffer *cmd, u32 x, u32 y, u32 z)
{
	vkCmdDispatch(cmd->handle, x, y, z);
}

internal void
GFX_CmdDispatchIndirect(const GFX_CmdBuffer *cmd, const GFX_Buffer *buffer, u64 offset)
{
	vkCmdDispatchIndirect(cmd->handle, buffer->handle, offset);
}

internal void
GFX_CmdBlit(const GFX_CmdBuffer *cmd,
			const GFX_Texture *src,
			const GFX_Texture *dst,
			u32 region_count, const VkImageBlit2 *regions,
			VkFilter filter)
{
	VkBlitImageInfo2 info = {0};
	info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
	info.srcImage = src->handle;
	info.dstImage = dst->handle;
	info.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.regionCount = region_count;
	info.pRegions = regions;
	info.filter = filter;

	vkCmdBlitImage2(cmd->handle, &info);
}

internal void
GFX_CmdGenerateMipmaps(const GFX_CmdBuffer *cmd, const GFX_Texture *texture)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = texture->handle;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = texture->aspect_flags;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = texture->layer_count;
	barrier.subresourceRange.levelCount = 1;

	for (u32 i = 1; i < texture->mipmap_count; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

		GFX_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &barrier);

		for (u32 face = 0; face < texture->layer_count; face++)
		{
			i32 src_mip_width  = (i32)texture->width  >> (i - 1);
			i32 src_mip_height = (i32)texture->height >> (i - 1);
			i32 dst_mip_width  = (i32)texture->width  >> (i - 0);
			i32 dst_mip_height = (i32)texture->height >> (i - 0);

			VkImageBlit2 blit = {0};
			blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

			blit.srcOffsets[0] = (VkOffset3D) { 0, 0, 0 };
			blit.srcOffsets[1] = (VkOffset3D) { src_mip_width, src_mip_height, 1 };

			blit.dstOffsets[0] = (VkOffset3D) { 0, 0, 0 };
			blit.dstOffsets[1] = (VkOffset3D) { dst_mip_width, dst_mip_height, 1 };

			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;

			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;

			GFX_CmdBlit(cmd, texture, texture, 1, &blit, VK_FILTER_LINEAR);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		GFX_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &barrier);
	}

	barrier.subresourceRange.baseMipLevel = texture->mipmap_count - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

	GFX_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &barrier);
}

internal void
GFX_CmdCopyBufferToBuffer(const GFX_CmdBuffer *cmd,
						  const GFX_Buffer *src,
						  const GFX_Buffer *dst,
						  u32 region_count, const VkBufferCopy2 *regions)
{
	VkCopyBufferInfo2 copy_info = {0};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
	copy_info.srcBuffer = src->handle;
	copy_info.dstBuffer = dst->handle;
	copy_info.regionCount = region_count;
	copy_info.pRegions = regions;

	vkCmdCopyBuffer2(cmd->handle, &copy_info);
}

internal void
GFX_CmdCopyBufferToTexture(const GFX_CmdBuffer *cmd,
						   const GFX_Buffer *src,
						   const GFX_Texture *dst,
						   u32 region_count, const VkBufferImageCopy2 *regions)
{
	VkCopyBufferToImageInfo2 copy_info = {0};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.srcBuffer = src->handle;
	copy_info.dstImage = dst->handle;
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	copy_info.regionCount = region_count;
	copy_info.pRegions = regions;

	vkCmdCopyBufferToImage2(cmd->handle, &copy_info);
}

internal void
GFX_CmdCopyBufferToTextureWhole(const GFX_CmdBuffer *cmd,
								const GFX_Buffer *src,
								const GFX_Texture *dst,
								u64 buffer_offset)
{
	VkBufferImageCopy2 region = {0};
	region.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
	region.bufferOffset = buffer_offset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = (VkOffset3D) { 0, 0, 0 };
	region.imageExtent = (VkExtent3D) { dst->width, dst->height, 1 };

	GFX_CmdCopyBufferToTexture(cmd, src, dst, 1, &region);
}

internal void
GFX_CmdFillBuffer(const GFX_CmdBuffer *cmd,
				  const GFX_Buffer *buffer,
				  u64 offset, u64 size, u32 fill)
{
	vkCmdFillBuffer(cmd->handle,
					buffer->handle,
					offset, size, fill);
}

internal void
GFX_CmdBeginQuery(const GFX_CmdBuffer *cmd,
				  VkQueryPool pool,
				  u32 query, VkQueryControlFlags flags)
{
	vkCmdBeginQuery(cmd->handle, pool, query, flags);
}

internal void
GFX_CmdEndQuery(const GFX_CmdBuffer *cmd,
				VkQueryPool pool,
				u32 query)
{
	vkCmdEndQuery(cmd->handle, pool, query);
}

internal void
GFX_CmdResetQueries(const GFX_CmdBuffer *cmd,
					VkQueryPool pool,
					u32 first, u32 count)
{
	vkCmdResetQueryPool(cmd->handle, pool, first, count);
}

internal void
GFX_CmdWriteTimestamp(const GFX_CmdBuffer *cmd,
					  VkPipelineStageFlags2 stage,
					  VkQueryPool pool,
					  u32 index)
{
	vkCmdWriteTimestamp2(cmd->handle, stage, pool, index);
}
