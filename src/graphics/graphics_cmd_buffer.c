
static void G_CmdBegin(const G_CmdBuffer *cmd)
{
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	G_VK_CHECK(vkBeginCommandBuffer(cmd->vk_handle, &begin_info),
			   "Failed to begin recording instant command buffer.");
}

static void G_CmdEnd(const G_CmdBuffer *cmd)
{
	G_VK_CHECK(vkEndCommandBuffer(cmd->vk_handle),
			   "Failed to record command buffer.");
}

static void G_CmdBeginRendering(const G_CmdBuffer *cmd, const G_RenderInfo *info)
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
	// TODO: Just reset an static viewport and scissor variable
	//       and only *actually* set it at end_rendering()
	G_CmdSetViewport(cmd, viewport);
	G_CmdSetScissor(cmd, scissor);
}

static void G_CmdEndRendering(const G_CmdBuffer *cmd)
{
	vkCmdEndRendering(cmd->vk_handle);
}

static void G_CmdSetViewport(const G_CmdBuffer *cmd, VkViewport viewport)
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

static void G_CmdSetScissor(const G_CmdBuffer *cmd, VkRect2D scissor)
{
	vkCmdSetScissor(cmd->vk_handle, 0, 1, &scissor);
}

static void G_CmdPipelineBarrier(const G_CmdBuffer *cmd, VkDependencyFlags dependency_flags,
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

static void G_CmdBindDescriptors(const G_CmdBuffer *cmd,
								 VkShaderStageFlags stage_flags,
								 G_PipelineLayoutKey layout, u32 first,
								 u32 descriptor_count, const VkDescriptorSet *descriptors,
								 u32 dynamic_offset_count, const u32 *dynamic_offsets)
{
	VkPipelineLayout vk_layout = G_DevicePipelineLayoutFromKey(layout);
	
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

static void G_CmdBindBindless(const G_CmdBuffer *cmd,
							  VkShaderStageFlags stage_flags,
							  G_PipelineLayoutKey layout)
{
	VkPipelineLayout vk_layout = G_DevicePipelineLayoutFromKey(layout);
	
	VkBindDescriptorSetsInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = vk_layout;
	info.firstSet = 0;
	info.descriptorSetCount = 1;
	info.pDescriptorSets = &G_DeviceGetSelected()->bindless.set;
	info.dynamicOffsetCount = 0;
	info.pDynamicOffsets = NULL;

	vkCmdBindDescriptorSets2(cmd->vk_handle, &info);
}

static void G_CmdBindPipeline(const G_CmdBuffer *cmd,
							  VkPipelineBindPoint bind_point,
							  G_PipelineKey pipeline)
{
	VkPipeline vk_pipeline = G_DevicePipelineFromKey(pipeline);
	 
	vkCmdBindPipeline(cmd->vk_handle, bind_point, vk_pipeline);
}

// TODO: Take in G_BufferRange as input?
static void G_CmdBindIndexBuffer(const G_CmdBuffer *cmd,
								 G_BufferKey buffer,
								 u64 offset, u64 size,
								 VkIndexType type)
{
	G_Buffer *gfx_buffer = G_DeviceBufferFromKey(buffer);
	
	vkCmdBindIndexBuffer2(cmd->vk_handle,
						  gfx_buffer->vk_handle,
						  offset, size,
						  type);
}

static void G_CmdPushConstantsEx(const G_CmdBuffer *cmd,
								 G_PipelineLayoutKey layout,
								 VkShaderStageFlags stage_flags,
								 u64 size, const void *data,
								 u32 offset)
{
	// Vulkan 1.4 minimum is 256 bytes.
	AssertTrue(size <= Bytes(256));
	
	VkPipelineLayout vk_layout = G_DevicePipelineLayoutFromKey(layout);
	
	VkPushConstantsInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
	info.layout = vk_layout;
	info.stageFlags = stage_flags;
	info.offset = offset;
	info.size = size;
	info.pValues = data;

	vkCmdPushConstants2(cmd->vk_handle, &info);
}

static void G_CmdSetLineWidth(const G_CmdBuffer *cmd, f32 thickness)
{
	vkCmdSetLineWidth(cmd->vk_handle, thickness);
}

static void G_CmdDraw(const G_CmdBuffer *cmd,
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

static void G_CmdDrawIndexed(const G_CmdBuffer *cmd,
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

static void G_CmdDrawIndexedIndirect(const G_CmdBuffer *cmd,
									 G_BufferKey buffer, u64 offset,
									 u32 count, u32 stride)
{
	G_Buffer *gfx_buffer = G_DeviceBufferFromKey(buffer);
	
	vkCmdDrawIndexedIndirect(cmd->vk_handle,
							 gfx_buffer->vk_handle, offset,
							 count, stride);
}

static void G_CmdDrawIndexedIndirectCount(const G_CmdBuffer *cmd,
										  G_BufferKey indirect_buffer, u64 indirect_offset,
										  G_BufferKey count_buffer, u64 count_offset,
										  u32 max_count, u32 stride)
{
	G_Buffer *gfx_indirect_buffer = G_DeviceBufferFromKey(indirect_buffer);
	G_Buffer *gfx_count_buffer = G_DeviceBufferFromKey(count_buffer);
	
	vkCmdDrawIndexedIndirectCount(cmd->vk_handle,
								  gfx_indirect_buffer->vk_handle, indirect_offset,
								  gfx_count_buffer->vk_handle,    count_offset,
								  max_count, stride);
}

static void G_CmdDrawMeshTasksIndirectCount(const G_CmdBuffer *cmd,
											G_BufferKey indirect_buffer, u64 indirect_offset,
											G_BufferKey count_buffer, u64 count_offset,
											u32 max_count, u32 stride)
{
	G_Buffer *gfx_indirect_buffer = G_DeviceBufferFromKey(indirect_buffer);
	G_Buffer *gfx_count_buffer = G_DeviceBufferFromKey(count_buffer);
	
	vkCmdDrawMeshTasksIndirectCountEXT(cmd->vk_handle,
									   gfx_indirect_buffer->vk_handle, indirect_offset,
									   gfx_count_buffer->vk_handle,    count_offset,
									   max_count, stride);
}

static void G_CmdDispatch(const G_CmdBuffer *cmd, u32 x, u32 y, u32 z)
{
	vkCmdDispatch(cmd->vk_handle, x, y, z);
}

static void G_CmdDispatchIndirect(const G_CmdBuffer *cmd, G_BufferKey buffer, u64 offset)
{
	G_Buffer *gfx_buffer = G_DeviceBufferFromKey(buffer);
	
	vkCmdDispatchIndirect(cmd->vk_handle,
						  gfx_buffer->vk_handle,
						  offset);
}

static void G_CmdBlit(const G_CmdBuffer *cmd,
					  G_TextureKey src,
					  G_TextureKey dst,
					  u32 region_count, const VkImageBlit2 *regions,
					  VkFilter filter)
{
	G_Texture *gfx_src = G_DeviceTextureFromKey(src);
	G_Texture *gfx_dst = G_DeviceTextureFromKey(dst);
	
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

static void G_CmdGenerateMipmaps(const G_CmdBuffer *cmd, G_TextureKey texture)
{
	G_Texture *gfx_texture = G_DeviceTextureFromKey(texture);
	
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

		G_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &barrier);

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

			G_CmdBlit(cmd, texture, texture, 1, &blit, VK_FILTER_LINEAR);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		G_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &barrier);
	}

	barrier.subresourceRange.baseMipLevel = gfx_texture->mipmap_count - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

	G_CmdPipelineBarrier(cmd, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static void G_CmdCopyBufferToBuffer(const G_CmdBuffer *cmd,
									G_BufferKey src,
									G_BufferKey dst,
									u32 region_count, const G_BufferCopy *regions)
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
	
	G_Buffer *gfx_src = G_DeviceBufferFromKey(src);
	G_Buffer *gfx_dst = G_DeviceBufferFromKey(dst);
	
	VkCopyBufferInfo2 copy_info = {0};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
	copy_info.srcBuffer = gfx_src->vk_handle;
	copy_info.dstBuffer = gfx_dst->vk_handle;
	copy_info.regionCount = region_count;
	copy_info.pRegions = vk_regions;

	vkCmdCopyBuffer2(cmd->vk_handle, &copy_info);

	ScratchRelease(&scratch);
}

static void G_CmdCopyBufferToTexture(const G_CmdBuffer *cmd,
									 G_BufferKey src,
									 G_TextureKey dst,
									 u32 region_count, const G_BufferImageCopy *regions)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkBufferImageCopy2 *vk_regions = ArenaPushArray(scratch.arena, VkBufferImageCopy2, region_count);

	for (u32 i = 0; i < region_count; i++)
	{
		const G_BufferImageCopy *r = &regions[i];
		
		vk_regions[i].sType             = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
		vk_regions[i].bufferOffset      = r->buffer_offset;
		vk_regions[i].bufferRowLength   = r->buffer_row_length;
		vk_regions[i].bufferImageHeight = r->buffer_image_height;
		vk_regions[i].imageSubresource  = r->image_subresource;
		vk_regions[i].imageOffset       = (VkOffset3D) { r->x, r->y, r->z };
		vk_regions[i].imageExtent       = (VkExtent3D) { r->w, r->h, r->d };
	}
	
	G_Buffer *gfx_src = G_DeviceBufferFromKey(src);
	G_Texture *gfx_dst = G_DeviceTextureFromKey(dst);
	
	VkCopyBufferToImageInfo2 copy_info = {0};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.srcBuffer = gfx_src->vk_handle;
	copy_info.dstImage = gfx_dst->vk_handle;
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	copy_info.regionCount = region_count;
	copy_info.pRegions = vk_regions;

	vkCmdCopyBufferToImage2(cmd->vk_handle, &copy_info);
}

static void G_CmdCopyBufferToTextureWhole(const G_CmdBuffer *cmd,
										  G_BufferKey src,
										  G_TextureKey dst,
										  u64 buffer_offset)
{
	G_Texture *gfx_dst = G_DeviceTextureFromKey(dst);
	
	G_BufferImageCopy region = {0};

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
	
	G_CmdCopyBufferToTexture(cmd, src, dst, 1, &region);
}

static void G_CmdFillBuffer(const G_CmdBuffer *cmd,
							G_BufferKey buffer,
							u64 offset, u64 size, u32 fill)
{
	G_Buffer *gfx_buffer = G_DeviceBufferFromKey(buffer);
	
	vkCmdFillBuffer(cmd->vk_handle,
					gfx_buffer->vk_handle,
					offset, size, fill);
}

static void G_CmdBuildBLAS(const G_CmdBuffer *cmd,
						   G_AccelStructKey blas,
						   const G_BLASGeometry *geometries, u32 geometry_count,
						   G_BufferKey scratch_buffer)
{
	G_AccelStruct *accel_struct = G_DeviceAccelStructFromKey(blas);

	u64 scratch_address = G_DeviceBufferAddress(scratch_buffer);
	
	ScratchArena scratch = ScratchBegin(NULL, 0);

	VkAccelerationStructureGeometryKHR *vk_geometries = ArenaPushArray(scratch.arena, VkAccelerationStructureGeometryKHR, geometry_count);
	VkAccelerationStructureBuildRangeInfoKHR *ranges = ArenaPushArray(scratch.arena, VkAccelerationStructureBuildRangeInfoKHR, geometry_count);
	
	for (u32 i = 0; i < geometry_count; i++)
	{
		const G_BLASGeometry *geometry = &geometries[i];

		G_Buffer *vb = G_DeviceBufferFromKey(geometry->vertex_buffer);
		G_Buffer *ib = G_DeviceBufferFromKey(geometry->index_buffer);

		vk_geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vk_geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vk_geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureGeometryTrianglesDataKHR *tri = &vk_geometries[i].geometry.triangles;

		tri->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		tri->vertexFormat = geometry->vertex_format;
		tri->vertexData.deviceAddress = vb->device_address + geometry->vertex_offset;
		tri->vertexStride = geometry->vertex_stride;
		tri->maxVertex = geometry->vertex_count - 1;

		tri->indexType = geometry->index_type;
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

static void G_CmdBuildTLAS(const G_CmdBuffer *cmd,
						   G_AccelStructKey tlas,
						   G_BufferKey instance_buffer, u32 instance_count,
						   G_BufferKey scratch_buffer)
{
	G_AccelStruct *accel_struct = G_DeviceAccelStructFromKey(tlas);

	u64 instance_address = G_DeviceBufferAddress(instance_buffer);
	u64 scratch_address = G_DeviceBufferAddress(scratch_buffer);

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

static void G_CmdBeginQuery(const G_CmdBuffer *cmd,
							VkQueryPool pool,
							u32 query, VkQueryControlFlags flags)
{
	vkCmdBeginQuery(cmd->vk_handle, pool, query, flags);
}

static void G_CmdEndQuery(const G_CmdBuffer *cmd,
						  VkQueryPool pool,
						  u32 query)
{
	vkCmdEndQuery(cmd->vk_handle, pool, query);
}

static void G_CmdResetQueries(const G_CmdBuffer *cmd,
							  VkQueryPool pool,
							  u32 first, u32 count)
{
	vkCmdResetQueryPool(cmd->vk_handle, pool, first, count);
}

static void G_CmdWriteTimestamp(const G_CmdBuffer *cmd,
								VkPipelineStageFlags2 stage,
								VkQueryPool pool,
								u32 index)
{
	vkCmdWriteTimestamp2(cmd->vk_handle, stage, pool, index);
}

static void G_CmdBeginLabel(const G_CmdBuffer *cmd, String8 name)
{
	VkDebugUtilsLabelEXT label = {0};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = (const char *)name.str;

	vkCmdBeginDebugUtilsLabelEXT(cmd->vk_handle, &label);
}

static void G_CmdBeginLabelEx(const G_CmdBuffer *cmd, String8 name, v4 colour)
{
	VkDebugUtilsLabelEXT label = {0};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = (const char *)name.str;
	label.color[0] = colour.x;
	label.color[1] = colour.y;
	label.color[2] = colour.z;
	label.color[3] = colour.w;

	vkCmdBeginDebugUtilsLabelEXT(cmd->vk_handle, &label);
}

static void G_CmdEndLabel(const G_CmdBuffer *cmd)
{
	vkCmdEndDebugUtilsLabelEXT(cmd->vk_handle);
}
