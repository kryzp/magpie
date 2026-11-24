#include "graphics/cmd_buffer.h"
#include "graphics/device.h"
#include "graphics/pipeline.h"
#include "graphics/vk_check.h"
#include "container/vector.h"

void gfx::CmdBuffer::Begin() const
{
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	GRAPHICS_VK_CHECK(
		vkBeginCommandBuffer(vk_handle, &begin_info),
		"Failed to begin recording instant command buffer."
	);
}

void gfx::CmdBuffer::End() const
{
	GRAPHICS_VK_CHECK(
		vkEndCommandBuffer(vk_handle),
		"Failed to record command buffer."
	);
}

void gfx::CmdBuffer::BeginRendering(const RenderInfo &info) const
{
	VkRenderingInfo rendering_info = {};
	rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	rendering_info.renderArea.offset = VkOffset2D{ 0, 0 };
	rendering_info.renderArea.extent = VkExtent2D{ info.width, info.height };
	rendering_info.layerCount = 1;
	rendering_info.viewMask = info.view_mask;
	rendering_info.colorAttachmentCount = info.colour_attachment_count;
	rendering_info.pColorAttachments = info.colour_attachments;
	rendering_info.pDepthAttachment = info.depth_attachment.imageView ? &info.depth_attachment : nullptr;
	rendering_info.pStencilAttachment = nullptr;

	vkCmdBeginRendering(vk_handle, &rendering_info);

	VkViewport viewport = {};
	viewport.x = 0.f;
	viewport.y = 0.f;
	viewport.width = info.width;
	viewport.height = info.height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { info.width, info.height };

	// We have to update the dynamic states
	// every time we draw something.
	// TODO: Just reset an static viewport and scissor variable
	//       and only *actually* set it at end_rendering()
	SetViewport(viewport);
	SetScissor(scissor);
}

void gfx::CmdBuffer::EndRendering() const
{
	vkCmdEndRendering(vk_handle);
}

void gfx::CmdBuffer::SetViewport(VkViewport viewport) const
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

	vkCmdSetViewport(vk_handle, 0, 1, &corrected_viewport);
}

void gfx::CmdBuffer::SetScissor(VkRect2D scissor) const
{
	vkCmdSetScissor(vk_handle, 0, 1, &scissor);
}

void gfx::CmdBuffer::PipelineBarrier(
	VkDependencyFlags dependency_flags,
	unsigned memory_barrier_count, const VkMemoryBarrier2 *memory_barriers,
	unsigned buffer_barrier_count, const VkBufferMemoryBarrier2 *buffer_barriers,
	unsigned image_barrier_count, const VkImageMemoryBarrier2 *image_barriers
) const
{
	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.dependencyFlags = dependency_flags;

	dependency.memoryBarrierCount = memory_barrier_count;
	dependency.pMemoryBarriers = memory_barriers;

	dependency.bufferMemoryBarrierCount = buffer_barrier_count;
	dependency.pBufferMemoryBarriers = buffer_barriers;

	dependency.imageMemoryBarrierCount = image_barrier_count;
	dependency.pImageMemoryBarriers = image_barriers;

	vkCmdPipelineBarrier2(vk_handle, &dependency);
}

void gfx::CmdBuffer::BindDescriptors(
	VkShaderStageFlags stage_flags,
	ResourceKey layout, unsigned first,
	unsigned descriptor_count, const VkDescriptorSet *descriptors,
	unsigned dynamic_offset_count, const unsigned *dynamic_offsets
) const
{
	Device *device = Device::Get();

	VkPipelineLayout vk_layout = device->PipelineLayoutFromKey(layout);

	VkBindDescriptorSetsInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = vk_layout;
	info.firstSet = first;
	info.descriptorSetCount = descriptor_count;
	info.pDescriptorSets = descriptors;
	info.dynamicOffsetCount = dynamic_offset_count;
	info.pDynamicOffsets = dynamic_offsets;

	vkCmdBindDescriptorSets2KHR(vk_handle, &info);
}

void gfx::CmdBuffer::BindBindless(VkShaderStageFlags stage_flags, ResourceKey layout) const
{
	Device *device = Device::Get();

	VkPipelineLayout vk_layout = device->PipelineLayoutFromKey(layout);

	VkBindDescriptorSetsInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
	info.stageFlags = stage_flags;
	info.layout = vk_layout;
	info.firstSet = 0;
	info.descriptorSetCount = 1;
	info.pDescriptorSets = &device->bindless.set;
	info.dynamicOffsetCount = 0;
	info.pDynamicOffsets = nullptr;

	vkCmdBindDescriptorSets2KHR(vk_handle, &info);
}

void gfx::CmdBuffer::BindPipeline(VkPipelineBindPoint bind_point, ResourceKey pipeline) const
{
	Device *device = Device::Get();

	VkPipeline vk_pipeline = device->PipelineFromKey(pipeline);

	vkCmdBindPipeline(vk_handle, bind_point, vk_pipeline);
}

void gfx::CmdBuffer::BindPipelineState(const PipelineState &state) const
{
	VkShaderStageFlags stages = state.bind_point == VK_PIPELINE_BIND_POINT_COMPUTE
		? VK_SHADER_STAGE_COMPUTE_BIT
		: VK_SHADER_STAGE_ALL_GRAPHICS;

	BindBindless(stages, state.layout);
	BindPipeline(state.bind_point, state.pipeline);
}

// TODO: Take in a buffer range as input?
void gfx::CmdBuffer::BindIndexBuffer(
	ResourceKey buffer,
	u64 offset, u64 size,
	IndexType index_type
) const
{
	Device *device = Device::Get();

	Buffer *gfx_buffer = device->BufferFromKey(buffer);
	VkIndexType vk_index_type = IndexTypeToVk(index_type);

	vkCmdBindIndexBuffer2KHR(
		vk_handle,
		gfx_buffer->vk_handle,
		offset, size,
		vk_index_type
	);
}

void gfx::CmdBuffer::PushConstantsEx(
	ResourceKey layout,
	VkShaderStageFlags stage_flags,
	u64 size, const void *data,
	unsigned offset
) const
{
	Device *device = Device::Get();

	u64 max_push_constants_size = device->max_push_constants_size;

	DebugLogAssert(
		device->log_channel_cmd_buffer,
		size <= max_push_constants_size,
		"Push constants must not be over %llu bytes in size.",
		max_push_constants_size
	);

	VkPipelineLayout vk_layout = device->PipelineLayoutFromKey(layout);

	VkPushConstantsInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
	info.layout = vk_layout;
	info.stageFlags = stage_flags;
	info.offset = offset;
	info.size = size;
	info.pValues = data;

	vkCmdPushConstants2KHR(vk_handle, &info);
}

void gfx::CmdBuffer::SetLineWidth(float thickness) const
{
	vkCmdSetLineWidth(vk_handle, thickness);
}

void gfx::CmdBuffer::Draw(
	unsigned vertex_count,
	unsigned instance_count,
	unsigned first_vertex,
	unsigned first_instance
) const
{
	vkCmdDraw(
		vk_handle,
		vertex_count,
		instance_count,
		first_vertex,
		first_instance
	);
}

void gfx::CmdBuffer::DrawIndexed(
	unsigned index_count,
	unsigned instance_count,
	unsigned first_index,
	int vertex_offset,
	unsigned first_instance
) const
{
	vkCmdDrawIndexed(
		vk_handle,
		index_count,
		instance_count,
		first_index,
		vertex_offset,
		first_instance
	);
}

void gfx::CmdBuffer::DrawIndexedIndirect(
	ResourceKey buffer, u64 offset,
	unsigned count, unsigned stride
) const
{
	Device *device = Device::Get();

	Buffer *gfx_buffer = device->BufferFromKey(buffer);

	vkCmdDrawIndexedIndirect(
		vk_handle,
		gfx_buffer->vk_handle, offset,
		count, stride
	);
}

void gfx::CmdBuffer::DrawIndexedIndirectCount(
	ResourceKey indirect_buffer, u64 indirect_offset,
	ResourceKey count_buffer, u64 count_offset,
	unsigned max_count, unsigned stride
) const
{
	Device *device = Device::Get();

	Buffer *gfx_indirect_buffer = device->BufferFromKey(indirect_buffer);
	Buffer *gfx_count_buffer = device->BufferFromKey(count_buffer);

	vkCmdDrawIndexedIndirectCount(
		vk_handle,
		gfx_indirect_buffer->vk_handle, indirect_offset,
		gfx_count_buffer->vk_handle, count_offset,
		max_count, stride
	);
}

void gfx::CmdBuffer::DrawMeshTasksIndirectCount(
	ResourceKey indirect_buffer, u64 indirect_offset,
	ResourceKey count_buffer, u64 count_offset,
	unsigned max_count, unsigned stride
) const
{
	Device *device = Device::Get();

	Buffer *gfx_indirect_buffer = device->BufferFromKey(indirect_buffer);
	Buffer *gfx_count_buffer = device->BufferFromKey(count_buffer);

	vkCmdDrawMeshTasksIndirectCountEXT(
		vk_handle,
		gfx_indirect_buffer->vk_handle, indirect_offset,
		gfx_count_buffer->vk_handle, count_offset,
		max_count, stride
	);
}

void gfx::CmdBuffer::Dispatch(unsigned x, unsigned y, unsigned z) const
{
	vkCmdDispatch(vk_handle, x, y, z);
}

void gfx::CmdBuffer::DispatchIndirect(ResourceKey buffer, u64 offset) const
{
	Device *device = Device::Get();

	Buffer *gfx_buffer = device->BufferFromKey(buffer);

	vkCmdDispatchIndirect(
		vk_handle,
		gfx_buffer->vk_handle,
		offset
	);
}

void gfx::CmdBuffer::Blit(
	ResourceKey src_texture,
	ResourceKey dst_texture,
	unsigned region_count, const VkImageBlit2 *regions,
	VkFilter filter
) const
{
	Device *device = Device::Get();

	Texture *gfx_src = device->TextureFromKey(src_texture);
	Texture *gfx_dst = device->TextureFromKey(dst_texture);

	VkBlitImageInfo2 info = {};
	info.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
	info.srcImage = gfx_src->vk_handle;
	info.dstImage = gfx_dst->vk_handle;
	info.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.regionCount = region_count;
	info.pRegions = regions;
	info.filter = filter;

	vkCmdBlitImage2(vk_handle, &info);
}

void gfx::CmdBuffer::GenerateMipmaps(ResourceKey texture) const
{
	Device *device = Device::Get();

	Texture *gfx_texture = device->TextureFromKey(texture);

	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = gfx_texture->vk_handle;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = gfx_texture->aspect_flags;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = gfx_texture->layer_count;
	barrier.subresourceRange.levelCount = 1;

	for (unsigned i = 1; i < gfx_texture->mipmap_count; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;

		PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &barrier);

		for (unsigned face = 0; face < gfx_texture->layer_count; face++) {
			int src_mip_width = (int)gfx_texture->width >> (i - 1);
			int src_mip_height = (int)gfx_texture->height >> (i - 1);
			int dst_mip_width = (int)gfx_texture->width >> (i - 0);
			int dst_mip_height = (int)gfx_texture->height >> (i - 0);

			VkImageBlit2 blit = {};
			blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;

			blit.srcOffsets[0] = VkOffset3D{ 0, 0, 0 };
			blit.srcOffsets[1] = VkOffset3D{ src_mip_width, src_mip_height, 1 };

			blit.dstOffsets[0] = VkOffset3D{ 0, 0, 0 };
			blit.dstOffsets[1] = VkOffset3D{ dst_mip_width, dst_mip_height, 1 };

			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;

			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;

			Blit(texture, texture, 1, &blit, VK_FILTER_LINEAR);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

		PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	barrier.subresourceRange.baseMipLevel = gfx_texture->mipmap_count - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

	barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_BLIT_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

	PipelineBarrier(0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void gfx::CmdBuffer::CopyBufferToBuffer(
	ResourceKey src_buffer,
	ResourceKey dst_buffer,
	unsigned region_count,
	const BufferCopy *regions
) const
{
	Device *device = Device::Get();

	Vector<VkBufferCopy2> vk_regions(region_count);

	for (unsigned i = 0; i < region_count; i++) {
		vk_regions[i].sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		vk_regions[i].srcOffset = regions[i].src_offset;
		vk_regions[i].dstOffset = regions[i].dst_offset;
		vk_regions[i].size = regions[i].size;
	}

	Buffer *gfx_src = device->BufferFromKey(src_buffer);
	Buffer *gfx_dst = device->BufferFromKey(dst_buffer);

	VkCopyBufferInfo2 copy_info = {};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
	copy_info.srcBuffer = gfx_src->vk_handle;
	copy_info.dstBuffer = gfx_dst->vk_handle;
	copy_info.regionCount = region_count;
	copy_info.pRegions = vk_regions.data();

	vkCmdCopyBuffer2(vk_handle, &copy_info);
}

void gfx::CmdBuffer::CopyBufferToTexture(
	ResourceKey src_buffer,
	ResourceKey dst_texture,
	unsigned region_count,
	const BufferImageCopy *regions
) const
{
	Device *device = Device::Get();

	Vector<VkBufferImageCopy2> vk_regions(region_count);

	for (unsigned i = 0; i < region_count; i++) {
		const BufferImageCopy *r = &regions[i];

		vk_regions[i].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
		vk_regions[i].bufferOffset = r->buffer_offset;
		vk_regions[i].bufferRowLength = r->buffer_row_length;
		vk_regions[i].bufferImageHeight = r->buffer_image_height;
		vk_regions[i].imageSubresource = r->image_subresource;
		vk_regions[i].imageOffset = VkOffset3D{ r->x, r->y, r->z };
		vk_regions[i].imageExtent = VkExtent3D{ r->width, r->height, r->depth };
	}

	Buffer *gfx_src = device->BufferFromKey(src_buffer);
	Texture *gfx_dst = device->TextureFromKey(dst_texture);

	VkCopyBufferToImageInfo2 copy_info = {};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
	copy_info.srcBuffer = gfx_src->vk_handle;
	copy_info.dstImage = gfx_dst->vk_handle;
	copy_info.dstImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	copy_info.regionCount = region_count;
	copy_info.pRegions = vk_regions.data();

	vkCmdCopyBufferToImage2(vk_handle, &copy_info);
}

void gfx::CmdBuffer::CopyTextureToBuffer(
	ResourceKey src_texture,
	ResourceKey dst_buffer,
	unsigned region_count,
	const BufferImageCopy *regions
) const
{
	Device *device = Device::Get();

	Vector<VkBufferImageCopy2> vk_regions(region_count);

	for (unsigned i = 0; i < region_count; i++) {
		const BufferImageCopy *r = &regions[i];

		vk_regions[i].sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
		vk_regions[i].bufferOffset = r->buffer_offset;
		vk_regions[i].bufferRowLength = r->buffer_row_length;
		vk_regions[i].bufferImageHeight = r->buffer_image_height;
		vk_regions[i].imageSubresource = r->image_subresource;
		vk_regions[i].imageOffset = VkOffset3D{ r->x, r->y, r->z };
		vk_regions[i].imageExtent = VkExtent3D{ r->width, r->height, r->depth };
	}

	Texture *gfx_src = device->TextureFromKey(src_texture);
	Buffer *gfx_dst = device->BufferFromKey(dst_buffer);

	VkCopyImageToBufferInfo2 copy_info = {};
	copy_info.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_TO_BUFFER_INFO_2;
	copy_info.srcImage = gfx_src->vk_handle;
	copy_info.srcImageLayout = VK_IMAGE_LAYOUT_GENERAL;
	copy_info.dstBuffer = gfx_dst->vk_handle;
	copy_info.regionCount = region_count;
	copy_info.pRegions = vk_regions.data();

	vkCmdCopyImageToBuffer2(vk_handle, &copy_info);
}

void gfx::CmdBuffer::CopyBufferToTextureWhole(
	ResourceKey src_buffer,
	ResourceKey dst_texture,
	u64 buffer_offset
) const
{
	Device *device = Device::Get();

	Texture *gfx_dst = device->TextureFromKey(dst_texture);

	BufferImageCopy region = {};

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

	region.width = gfx_dst->width;
	region.height = gfx_dst->height;
	region.depth = 1;

	CopyBufferToTexture(src_buffer, dst_texture, 1, &region);
}

void gfx::CmdBuffer::FillBuffer(
	ResourceKey buffer,
	u64 offset,
	u64 size,
	unsigned fill
) const
{
	Device *device = Device::Get();

	Buffer *gfx_buffer = device->BufferFromKey(buffer);

	vkCmdFillBuffer(
		vk_handle,
		gfx_buffer->vk_handle,
		offset, size, fill
	);
}

void gfx::CmdBuffer::BuildBLAS(
	ResourceKey blas,
	const BLASGeometry *geometries,
	unsigned geometry_count,
	ResourceKey scratch_buffer
) const
{
	Device *device = Device::Get();

	AccelStruct *accel_struct = device->AccelStructFromKey(blas);

	u64 scratch_address = device->BufferAddress(scratch_buffer);

	Vector<VkAccelerationStructureGeometryKHR> vk_geometries(geometry_count);
	Vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(geometry_count);

	for (unsigned i = 0; i < geometry_count; i++) {
		const BLASGeometry *geometry = &geometries[i];

		Buffer *vb = device->BufferFromKey(geometry->vertex_buffer);
		Buffer *ib = device->BufferFromKey(geometry->index_buffer);

		vk_geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		vk_geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		vk_geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		VkAccelerationStructureGeometryTrianglesDataKHR *tri = &vk_geometries[i].geometry.triangles;

		tri->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

		tri->vertexFormat = geometry->vertex_format;
		tri->vertexData.deviceAddress = vb->device_address + geometry->vertex_offset;
		tri->vertexStride = geometry->vertex_stride;
		tri->maxVertex = geometry->vertex_count - 1;

		tri->indexType = IndexTypeToVk(geometry->index_type);
		tri->indexData.deviceAddress = ib->device_address + geometry->index_offset;

		ranges[i].primitiveCount = geometry->index_count / 3;
		ranges[i].primitiveOffset = 0;
		ranges[i].firstVertex = 0;
	}

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.dstAccelerationStructure = accel_struct->vk_handle;
	build_info.geometryCount = geometry_count;
	build_info.pGeometries = vk_geometries.data();
	build_info.scratchData.deviceAddress = scratch_address;

	const VkAccelerationStructureBuildRangeInfoKHR *range_ptrs = ranges.data();

	vkCmdBuildAccelerationStructuresKHR(vk_handle, 1, &build_info, &range_ptrs);
}

void gfx::CmdBuffer::BuildTLAS(
	ResourceKey tlas,
	ResourceKey instance_buffer,
	unsigned instance_count,
	ResourceKey scratch_buffer
) const
{
	Device *device = Device::Get();

	AccelStruct *accel_struct = device->AccelStructFromKey(tlas);

	u64 instance_address = device->BufferAddress(instance_buffer);
	u64 scratch_address = device->BufferAddress(scratch_buffer);

	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	geometry.geometry.instances.data.deviceAddress = instance_address;

	VkAccelerationStructureBuildGeometryInfoKHR build_info = {};
	build_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_info.dstAccelerationStructure = accel_struct->vk_handle;
	build_info.geometryCount = 1;
	build_info.pGeometries = &geometry;
	build_info.scratchData.deviceAddress = scratch_address;

	VkAccelerationStructureBuildRangeInfoKHR range = {};
	range.primitiveCount = instance_count;

	const VkAccelerationStructureBuildRangeInfoKHR *range_ptr = &range;

	vkCmdBuildAccelerationStructuresKHR(vk_handle, 1, &build_info, &range_ptr);
}

void gfx::CmdBuffer::BeginQuery(
	VkQueryPool pool,
	unsigned query, VkQueryControlFlags flags
) const
{
	vkCmdBeginQuery(vk_handle, pool, query, flags);
}

void gfx::CmdBuffer::EndQuery(
	VkQueryPool pool,
	unsigned query
) const
{
	vkCmdEndQuery(vk_handle, pool, query);
}

void gfx::CmdBuffer::ResetQueries(
	VkQueryPool pool,
	unsigned first, unsigned count
) const
{
	vkCmdResetQueryPool(vk_handle, pool, first, count);
}

void gfx::CmdBuffer::WriteTimestamp(
	VkPipelineStageFlags2 stage,
	VkQueryPool pool,
	unsigned index
) const
{
	vkCmdWriteTimestamp2(vk_handle, stage, pool, index);
}

void gfx::CmdBuffer::BeginLabel(const char *name) const
{
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = name;

	vkCmdBeginDebugUtilsLabelEXT(vk_handle, &label);
}

void gfx::CmdBuffer::BeginLabel(const char *name, const Vec4 &colour) const
{
	VkDebugUtilsLabelEXT label = {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = name;
	label.color[0] = colour.x;
	label.color[1] = colour.y;
	label.color[2] = colour.z;
	label.color[3] = colour.w;

	vkCmdBeginDebugUtilsLabelEXT(vk_handle, &label);
}

void gfx::CmdBuffer::EndLabel() const
{
	vkCmdEndDebugUtilsLabelEXT(vk_handle);
}
