
internal void
GFX_CmdBegin(const GFX_CmdBuffer *cmd)
{
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	GFX_VK_CHECK(vkBeginCommandBuffer(cmd->vk_handle, &begin_info),
				 "Failed to begin recording instant command buffer.");
}

internal void
GFX_CmdEnd(const GFX_CmdBuffer *cmd)
{
	GFX_VK_CHECK(vkEndCommandBuffer(cmd->vk_handle),
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

	vkCmdBeginRendering(cmd->vk_handle, &rendering_info);

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
	vkCmdEndRendering(cmd->vk_handle);
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

	vkCmdSetViewport(cmd->vk_handle, 0, 1, &corrected_viewport);
}

internal void
GFX_CmdSetScissor(const GFX_CmdBuffer *cmd, VkRect2D scissor)
{
	vkCmdSetScissor(cmd->vk_handle, 0, 1, &scissor);
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

	vkCmdPipelineBarrier2(cmd->vk_handle, &dependency);
}

internal void
GFX_CmdBindDescriptors(const GFX_CmdBuffer *cmd,
					   VkShaderStageFlags stage_flags,
					   GFX_PipelineLayoutKey layout, u32 first,
					   u32 descriptor_count, const VkDescriptorSet *descriptors,
					   u32 dynamic_offset_count, const u32 *dynamic_offsets)
{
	VkPipelineLayout vk_layout = GFX_DevicePipelineLayoutFromKey(cmd->device, layout);
	
	VkBindDescriptorSetsInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = vk_layout;
	info.firstSet = first;
	info.descriptorSetCount = descriptor_count;
	info.pDescriptorSets = descriptors;
	info.dynamicOffsetCount = dynamic_offset_count;
	info.pDynamicOffsets = dynamic_offsets;

	vkCmdBindDescriptorSets2(cmd->vk_handle, &info);
}

internal void
GFX_CmdBindBindless(const GFX_CmdBuffer *cmd,
					VkShaderStageFlags stage_flags,
					GFX_PipelineLayoutKey layout)
{
	VkPipelineLayout vk_layout = GFX_DevicePipelineLayoutFromKey(cmd->device, layout);
	
	VkBindDescriptorSetsInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = vk_layout;
	info.firstSet = 0;
	info.descriptorSetCount = 1;
	info.pDescriptorSets = &cmd->device->bindless.set;
	info.dynamicOffsetCount = 0;
	info.pDynamicOffsets = NULL;

	vkCmdBindDescriptorSets2(cmd->vk_handle, &info);
}

internal void
GFX_CmdBindPipeline(const GFX_CmdBuffer *cmd,
					VkPipelineBindPoint bind_point,
					GFX_PipelineKey pipeline)
{
	VkPipeline vk_pipeline = GFX_DevicePipelineFromKey(cmd->device, pipeline);
	
	vkCmdBindPipeline(cmd->vk_handle, bind_point, vk_pipeline);
}

// TODO: Take in GFX_BufferRange as input?
internal void
GFX_CmdBindIndexBuffer(const GFX_CmdBuffer *cmd,
					   GFX_BufferKey buffer,
					   u64 offset, u64 size,
					   VkIndexType type)
{
	GFX_Buffer *gfx_buffer = GFX_DeviceBufferFromKey(cmd->device, buffer);
	
	vkCmdBindIndexBuffer2(cmd->vk_handle,
						  gfx_buffer->vk_handle,
						  offset, size,
						  type);
}

internal void
GFX_CmdPushConstants(const GFX_CmdBuffer *cmd,
					 GFX_PipelineLayoutKey layout,
					 VkShaderStageFlags stage_flags,
					 u64 size, const void *data,
					 u32 offset)
{
	// Vulkan 1.4 minimum is 256 bytes.
	AssertTrue(size <= Bytes(256));
	
	VkPipelineLayout vk_layout = GFX_DevicePipelineLayoutFromKey(cmd->device, layout);
	
	VkPushConstantsInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
	info.layout = vk_layout;
	info.stageFlags = stage_flags;
	info.offset = offset;
	info.size = size;
	info.pValues = data;

	vkCmdPushConstants2(cmd->vk_handle, &info);
}

internal void
GFX_CmdSetLineWidth(const GFX_CmdBuffer *cmd, f32 thickness)
{
	vkCmdSetLineWidth(cmd->vk_handle, thickness);
}

internal void
GFX_CmdDraw(const GFX_CmdBuffer *cmd,
			u32 vertex_count,
			u32 instance_count,
			u32 first_vertex,
			u32 first_instance)
{
	vkCmdDraw(cmd->vk_handle,
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
	vkCmdDrawIndexed(cmd->vk_handle,
					 index_count,
					 instance_count,
					 first_index,
					 vertex_offset,
					 first_instance);
}

internal void
GFX_CmdDrawIndexedIndirect(const GFX_CmdBuffer *cmd,
						   GFX_BufferKey buffer, u64 offset,
						   u32 count, u32 stride)
{
	GFX_Buffer *gfx_buffer = GFX_DeviceBufferFromKey(cmd->device, buffer);
	
	vkCmdDrawIndexedIndirect(cmd->vk_handle,
							 gfx_buffer->vk_handle, offset,
							 count, stride);
}

internal void
GFX_CmdDrawIndexedIndirectCount(const GFX_CmdBuffer *cmd,
								GFX_BufferKey indirect_buffer, u64 indirect_offset,
								GFX_BufferKey count_buffer, u64 count_offset,
								u32 max_count, u32 stride)
{
	GFX_Buffer *gfx_indirect_buffer = GFX_DeviceBufferFromKey(cmd->device, indirect_buffer);
	GFX_Buffer *gfx_count_buffer    = GFX_DeviceBufferFromKey(cmd->device, count_buffer);
	
	vkCmdDrawIndexedIndirectCount(cmd->vk_handle,
								  gfx_indirect_buffer->vk_handle, indirect_offset,
								  gfx_count_buffer->vk_handle,    count_offset,
								  max_count, stride);
}

internal void
GFX_CmdDrawMeshTasksIndirectCount(const GFX_CmdBuffer *cmd,
								  GFX_BufferKey indirect_buffer, u64 indirect_offset,
								  GFX_BufferKey count_buffer, u64 count_offset,
								  u32 max_count, u32 stride)
{
	GFX_Buffer *gfx_indirect_buffer = GFX_DeviceBufferFromKey(cmd->device, indirect_buffer);
	GFX_Buffer *gfx_count_buffer    = GFX_DeviceBufferFromKey(cmd->device, count_buffer);
	
	vkCmdDrawMeshTasksIndirectCountEXT(cmd->vk_handle,
									   gfx_indirect_buffer->vk_handle, indirect_offset,
									   gfx_count_buffer->vk_handle,    count_offset,
									   max_count, stride);
}

internal void
GFX_CmdDispatch(const GFX_CmdBuffer *cmd, u32 x, u32 y, u32 z)
{
	vkCmdDispatch(cmd->vk_handle, x, y, z);
}

internal void
GFX_CmdDispatchIndirect(const GFX_CmdBuffer *cmd, GFX_BufferKey buffer, u64 offset)
{
	GFX_Buffer *gfx_buffer = GFX_DeviceBufferFromKey(cmd->device, buffer);
	
	vkCmdDispatchIndirect(cmd->vk_handle,
						  gfx_buffer->vk_handle,
						  offset);
}

internal void
GFX_CmdBlit(const GFX_CmdBuffer *cmd,
			GFX_TextureKey src,
			GFX_TextureKey dst,
			u32 region_count, const VkImageBlit2 *regions,
			VkFilter filter)
{
	GFX_Texture *gfx_src = GFX_DeviceTextureFromKey(cmd->device, src);
	GFX_Texture *gfx_dst = GFX_DeviceTextureFromKey(cmd->device, dst);
	
	VkBlitImageInfo2 info = {0};
	info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
	info.srcImage = gfx_src->vk_handle;
	info.dstImage = gfx_dst->vk_handle;
	info.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.regionCount = region_count;
	info.pRegions = regions;
	info.filter = filter;

	vkCmdBlitImage2(cmd->vk_handle, &info);
}

internal void
GFX_CmdGenerateMipmaps(const GFX_CmdBuffer *cmd, GFX_TextureKey texture)
{
	GFX_Texture *gfx_texture = GFX_DeviceTextureFromKey(cmd->device, texture);
	
	VkImageMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = gfx_texture->vk_handle;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = gfx_texture->aspect_flags;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = gfx_texture->layer_count;
	barrier.subresourceRange.levelCount = 1;

	for (u32 i = 1; i < gfx_texture->mipmap_count; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

		GFX_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &barrier);

		for (u32 face = 0; face < gfx_texture->layer_count; face++)
		{
			i32 src_mip_width  = (i32)gfx_texture->width  >> (i - 1);
			i32 src_mip_height = (i32)gfx_texture->height >> (i - 1);
			i32 dst_mip_width  = (i32)gfx_texture->width  >> (i - 0);
			i32 dst_mip_height = (i32)gfx_texture->height >> (i - 0);

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

	barrier.subresourceRange.baseMipLevel = gfx_texture->mipmap_count - 1;

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
						  GFX_BufferKey src,
						  GFX_BufferKey dst,
						  u32 region_count, const GFX_BufferCopy *regions)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkBufferCopy2 *vk_regions = ArenaPushArray(scratch.arena, VkBufferCopy2, region_count);

	for (u32 i = 0; i < region_count; i++)
	{
		vk_regions[i].sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		vk_regions[i].srcOffset = regions[i].src_offset;
		vk_regions[i].dstOffset = regions[i].dst_offset;
		vk_regions[i].size      = regions[i].size;
	}
	
	GFX_Buffer *gfx_src = GFX_DeviceBufferFromKey(cmd->device, src);
	GFX_Buffer *gfx_dst = GFX_DeviceBufferFromKey(cmd->device, dst);
	
	VkCopyBufferInfo2 copy_info = {0};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
	copy_info.srcBuffer = gfx_src->vk_handle;
	copy_info.dstBuffer = gfx_dst->vk_handle;
	copy_info.regionCount = region_count;
	copy_info.pRegions = vk_regions;

	vkCmdCopyBuffer2(cmd->vk_handle, &copy_info);

	ScratchRelease(&scratch);
}

internal void
GFX_CmdCopyBufferToTexture(const GFX_CmdBuffer *cmd,
						   GFX_BufferKey src,
						   GFX_TextureKey dst,
						   u32 region_count, const GFX_BufferImageCopy *regions)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkBufferImageCopy2 *vk_regions = ArenaPushArray(scratch.arena, VkBufferImageCopy2, region_count);

	for (u32 i = 0; i < region_count; i++)
	{
		const GFX_BufferImageCopy *r = &regions[i];
		
		vk_regions[i].sType             = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
		vk_regions[i].bufferOffset      = r->buffer_offset;
		vk_regions[i].bufferRowLength   = r->buffer_row_length;
		vk_regions[i].bufferImageHeight = r->buffer_image_height;
		vk_regions[i].imageSubresource  = r->image_subresource;
		vk_regions[i].imageOffset       = (VkOffset3D) { r->x, r->y, r->z };
		vk_regions[i].imageExtent       = (VkExtent3D) { r->w, r->h, r->d };
	}
	
	GFX_Buffer  *gfx_src = GFX_DeviceBufferFromKey  (cmd->device, src);
	GFX_Texture *gfx_dst = GFX_DeviceTextureFromKey (cmd->device, dst);
	
	VkCopyBufferToImageInfo2 copy_info = {0};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.srcBuffer = gfx_src->vk_handle;
	copy_info.dstImage = gfx_dst->vk_handle;
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	copy_info.regionCount = region_count;
	copy_info.pRegions = vk_regions;

	vkCmdCopyBufferToImage2(cmd->vk_handle, &copy_info);
}

internal void
GFX_CmdCopyBufferToTextureWhole(const GFX_CmdBuffer *cmd,
								GFX_BufferKey src,
								GFX_TextureKey dst,
								u64 buffer_offset)
{
	GFX_Texture *gfx_dst = GFX_DeviceTextureFromKey(cmd->device, dst);
	
	GFX_BufferImageCopy region = {0};

	region.buffer_offset = buffer_offset;
	region.buffer_row_length = 0;
	region.buffer_image_height = 0;

	region.image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.image_subresource.mipLevel = 0;
	region.image_subresource.baseArrayLayer = 0;
	region.image_subresource.layerCount = 1;

	region.x = 0;
	region.y = 0;
	region.z = 0;

	region.w = gfx_dst->width;
	region.h = gfx_dst->height;
	region.d = 1;
	
	GFX_CmdCopyBufferToTexture(cmd, src, dst, 1, &region);
}

internal void
GFX_CmdFillBuffer(const GFX_CmdBuffer *cmd,
				  GFX_BufferKey buffer,
				  u64 offset, u64 size, u32 fill)
{
	GFX_Buffer *gfx_buffer = GFX_DeviceBufferFromKey(cmd->device, buffer);
	
	vkCmdFillBuffer(cmd->vk_handle,
					gfx_buffer->vk_handle,
					offset, size, fill);
}

internal void
GFX_CmdBuildBLAS(const GFX_CmdBuffer *cmd,
				 GFX_AccelStructKey blas,
				 const GFX_BLASGeometry *geometries, u32 geometry_count,
				 GFX_BufferKey scratch_buffer)
{
	GFX_Device *device = cmd->device;

	GFX_AccelStruct *accel_struct = GFX_DeviceAccelStructFromKey(device, blas);

	u64 scratch_address = GFX_DeviceBufferAddress(device, scratch_buffer);
	
	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkAccelerationStructureGeometryKHR *vk_geometries = ArenaPushArray(scratch.arena, VkAccelerationStructureGeometryKHR, geometry_count);
	VkAccelerationStructureBuildRangeInfoKHR *ranges = ArenaPushArray(scratch.arena, VkAccelerationStructureBuildRangeInfoKHR, geometry_count);
	
	for (u32 i = 0; i < geometry_count; i++)
	{
		const GFX_BLASGeometry *geometry = &geometries[i];

		GFX_Buffer *vb = GFX_DeviceBufferFromKey(device, geometry->vertex_buffer);
		GFX_Buffer *ib = GFX_DeviceBufferFromKey(device, geometry->index_buffer);

		vk_geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vk_geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vk_geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureGeometryTrianglesDataKHR *tri = &vk_geometries[i].geometry.triangles;

		tri->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		tri->vertexFormat             = geometry->vertex_format;
		tri->vertexData.deviceAddress = vb->device_address + geometry->vertex_offset;
		tri->vertexStride             = geometry->vertex_stride;
		tri->maxVertex                = geometry->vertex_count - 1;

		tri->indexType               = geometry->index_type;
		tri->indexData.deviceAddress = ib->device_address + geometry->index_offset;

		ranges[i].primitiveCount = geometry->index_count / 3;
		ranges[i].primitiveOffset = 0;
		ranges[i].firstVertex = 0;
	}

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {0};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.dstAccelerationStructure = accel_struct->vk_handle;
	build_info.geometryCount = geometry_count;
	build_info.pGeometries = vk_geometries;
	build_info.scratchData.deviceAddress = scratch_address;

	const VkAccelerationStructureBuildRangeInfoKHR *range_ptrs = ranges;
	
	vkCmdBuildAccelerationStructuresKHR(cmd->vk_handle, 1, &build_info, &range_ptrs);
	
	ScratchRelease(&scratch);
}

internal void
GFX_CmdBuildTLAS(const GFX_CmdBuffer *cmd,
				 GFX_AccelStructKey tlas,
				 GFX_BufferKey instance_buffer, u32 instance_count,
				 GFX_BufferKey scratch_buffer)
{
	GFX_Device *device = cmd->device;

	GFX_AccelStruct *accel_struct = GFX_DeviceAccelStructFromKey(device, tlas);

	u64 instance_address = GFX_DeviceBufferAddress(device, instance_buffer);
	u64 scratch_address  = GFX_DeviceBufferAddress(device, scratch_buffer);

	VkAccelerationStructureGeometryKHR geometry = {0};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	geometry.geometry.instances.data.deviceAddress = instance_address;

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {0};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.dstAccelerationStructure = accel_struct->vk_handle;
	build_info.geometryCount = 1;
	build_info.pGeometries = &geometry;
	build_info.scratchData.deviceAddress = scratch_address;

	VkAccelerationStructureBuildRangeInfoKHR range = {0};
	range.primitiveCount = instance_count;

	const VkAccelerationStructureBuildRangeInfoKHR *range_ptr = &range;
	
	vkCmdBuildAccelerationStructuresKHR(cmd->vk_handle, 1, &build_info, &range_ptr);
}

internal void
GFX_CmdBeginQuery(const GFX_CmdBuffer *cmd,
				  VkQueryPool pool,
				  u32 query, VkQueryControlFlags flags)
{
	vkCmdBeginQuery(cmd->vk_handle, pool, query, flags);
}

internal void
GFX_CmdEndQuery(const GFX_CmdBuffer *cmd,
				VkQueryPool pool,
				u32 query)
{
	vkCmdEndQuery(cmd->vk_handle, pool, query);
}

internal void
GFX_CmdResetQueries(const GFX_CmdBuffer *cmd,
					VkQueryPool pool,
					u32 first, u32 count)
{
	vkCmdResetQueryPool(cmd->vk_handle, pool, first, count);
}

internal void
GFX_CmdWriteTimestamp(const GFX_CmdBuffer *cmd,
					  VkPipelineStageFlags2 stage,
					  VkQueryPool pool,
					  u32 index)
{
	vkCmdWriteTimestamp2(cmd->vk_handle, stage, pool, index);
}
