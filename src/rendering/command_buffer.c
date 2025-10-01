#include "command_buffer.h"
#include "device.h"

void gfx_cmd_begin(struct gfx_command_buffer *cmd)
{
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	GFX_VK_CHECK(vkBeginCommandBuffer(cmd->handle, &begin_info),
		 "Failed to begin recording instant command buffer.");
}

void gfx_cmd_end(struct gfx_command_buffer *cmd)
{
	GFX_VK_CHECK(vkEndCommandBuffer(cmd->handle),
		 "Failed to record command buffer.");
}

void gfx_cmd_set_viewport(struct gfx_command_buffer *cmd, VkViewport viewport)
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

void gfx_cmd_set_scissor(struct gfx_command_buffer *cmd, VkRect2D scissor)
{
	vkCmdSetScissor(cmd->handle, 0, 1, &scissor);
}

void gfx_cmd_begin_rendering(struct gfx_command_buffer *cmd, struct gfx_render_info *info)
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
	viewport.width = (float)info->width;
	viewport.height = (float)info->height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D){ 0, 0 };
	scissor.extent = (VkExtent2D){ info->width, info->height };

	// We have to update the dynamic states
	// every time we draw something.
	gfx_cmd_set_viewport(cmd, viewport);
	gfx_cmd_set_scissor(cmd, scissor);
}

void gfx_cmd_end_rendering(struct gfx_command_buffer *cmd)
{
	vkCmdEndRendering(cmd->handle);
}

void gfx_cmd_pipeline_barrier(struct gfx_command_buffer *cmd,
			      VkDependencyFlags dependency_flags,
			      u32 memory_barrier_count, VkMemoryBarrier2 *memory_barriers,
			      u32 buffer_memory_barrier_count, VkBufferMemoryBarrier2 *buffer_memory_barriers,
			      u32 image_memory_barrier_count, VkImageMemoryBarrier2 *image_memory_barriers)
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

void gfx_cmd_bind_descriptors(struct gfx_command_buffer *cmd,
			      VkPipelineBindPoint bind_point,
			      VkPipelineLayout layout, u32 first,
			      u32 descriptor_count, VkDescriptorSet *descriptors)
{
	vkCmdBindDescriptorSets(cmd->handle,
				bind_point,
				layout, first,
				descriptor_count,
				descriptors,
				0, NULL);
}

void gfx_cmd_bind_descriptors_dyoff(struct gfx_command_buffer *cmd,
				    VkPipelineBindPoint bind_point,
				    VkPipelineLayout layout, u32 first,
				    u32 descriptor_count, VkDescriptorSet *descriptors,
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

void gfx_cmd_bind_bindless(struct gfx_command_buffer *cmd,
			   VkPipelineBindPoint bind_point,
			   VkPipelineLayout layout,
			   struct gfx_device *device)
{
	vkCmdBindDescriptorSets(cmd->handle,
				bind_point,
				layout, 0,
				GFX_BINDLESS_SET_max_enum,
				device->bindless.sets,
				0, NULL);
}

void gfx_cmd_bind_pipeline(struct gfx_command_buffer *cmd,
			   VkPipelineBindPoint bind_point,
			   VkPipeline pipeline)
{
	vkCmdBindPipeline(cmd->handle,
			  bind_point,
			  pipeline);
}

void gfx_cmd_bind_index_buffer(struct gfx_command_buffer *cmd,
			       struct gfx_buffer *buffer,
			       u64 offset)
{
	vkCmdBindIndexBuffer(cmd->handle,
			     buffer->handle,
			     offset,
			     VK_INDEX_TYPE_UINT16);
}

void gfx_cmd_push_constants(struct gfx_command_buffer *cmd,
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

void gfx_cmd_push_constants_offset(struct gfx_command_buffer *cmd,
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

void gfx_cmd_draw_vertices_n(struct gfx_command_buffer *cmd,
			     u32 vertex_count)
{
	vkCmdDraw(cmd->handle, vertex_count, 1, 0, 0);
}

void gfx_cmd_draw_indexed(struct gfx_command_buffer *cmd,
			  u32 index_count,
			  u32 instance_count,
			  u32 first_index,
			  s32 vertex_offset,
			  u32 first_instance)
{
	vkCmdDrawIndexed(cmd->handle,
			 index_count,
			 instance_count,
			 first_index,
			 vertex_offset,
			 first_instance);
}

void gfx_cmd_draw_indexed_indirect(struct gfx_command_buffer *cmd,
				   struct gfx_buffer *buffer,
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

void gfx_cmd_blit(struct gfx_command_buffer *cmd,
		  struct gfx_texture *src,
		  struct gfx_texture *dst,
		  u32 region_count, VkImageBlit2 *regions,
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

void gfx_cmd_generate_mipmaps(struct gfx_command_buffer *cmd,
			      struct gfx_texture *texture)
{
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = texture->handle;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = texture->aspect_flags;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = gfx_texture_face_count(texture);
	barrier.subresourceRange.levelCount = 1;

	for (int i = 1; i < texture->mipmap_count; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

		gfx_cmd_pipeline_barrier(cmd, 0,
					 0, NULL,
					 0, NULL,
					 1, &barrier);

		for (int face = 0; face < gfx_texture_face_count(texture); face++) {
			int src_mip_width  = (int)texture->width  >> (i - 1);
			int src_mip_height = (int)texture->height >> (i - 1);
			int dst_mip_width  = (int)texture->width  >> (i - 0);
			int dst_mip_height = (int)texture->height >> (i - 0);

			VkImageBlit2 blit = {0};

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

			gfx_cmd_blit(cmd,
				     texture, texture,
				     1, &blit,
				     VK_FILTER_LINEAR);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		gfx_cmd_pipeline_barrier(cmd, 0,
					 0, NULL,
					 0, NULL,
					 1, &barrier);
		
		for (u32 k = 0; k < gfx_texture_face_count(texture); k++)
			gfx_texture_set_access_type(texture, i, k, 0, GFX_TEXTURE_ACCESS_TYPE_graphics_r);
	}

	barrier.subresourceRange.baseMipLevel = texture->mipmap_count - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	
	gfx_cmd_pipeline_barrier(cmd, 0,
				 0, NULL,
				 0, NULL,
				 1, &barrier);
	
	for (u32 k = 0; k < gfx_texture_face_count(texture); k++)
		gfx_texture_set_access_type(texture, 0, k, 0, GFX_TEXTURE_ACCESS_TYPE_graphics_r);
}

void gfx_cmd_copy_buffer_to_buffer(struct gfx_command_buffer *cmd,
				   struct gfx_buffer *src,
				   struct gfx_buffer *dst,
				   u32 region_count, VkBufferCopy *regions)
{
	vkCmdCopyBuffer(cmd->handle,
			src->handle,
			dst->handle,
			region_count, regions);
}

void gfx_cmd_copy_buffer_to_texture_multi_region(struct gfx_command_buffer *cmd,
						 struct gfx_buffer *buffer,
						 struct gfx_texture *texture,
						 u32 region_count, VkBufferImageCopy *regions)
{
	vkCmdCopyBufferToImage(cmd->handle,
			       buffer->handle,
			       texture->handle, VK_IMAGE_LAYOUT_GENERAL,
			       region_count, regions);
}

void gfx_cmd_copy_buffer_to_texture(struct gfx_command_buffer *cmd,
				    struct gfx_buffer *buffer,
				    struct gfx_texture *texture)
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
	region.imageExtent = (VkExtent3D){ texture->width, texture->height, 1 };

	gfx_cmd_copy_buffer_to_texture_multi_region(cmd,
						    buffer, texture,
						    1, &region);
}

void gfx_cmd_dispatch(struct gfx_command_buffer *cmd,
		      u32 x, u32 y, u32 z)
{
	vkCmdDispatch(cmd->handle,
		      x, y, z);
}
