
internal b32 GPUBufferIsUniform(GPUBuffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT) != 0;
}

internal b32 GPUBufferIsStorage(GPUBuffer *buffer)
{
	return (buffer->usage & VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT) != 0;
}

internal GPUBuffer GPUBufferAlloc(VkBufferUsageFlags2 usage,
				  VmaAllocationCreateFlagBits flags,
				  u64 size)
{
	GPUBuffer buffer = {0};

	buffer.usage = usage;
	buffer.size = size;
	buffer.allocation_flags = flags;

	if (GPUBufferIsStorage(&buffer))
		buffer.usage |= VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferCreateInfo buffer_create_info = {0};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = buffer.size;
	buffer_create_info.usage = buffer.usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;//queueFamilyIndices.size();
	buffer_create_info.pQueueFamilyIndices = 0;//queueFamilyIndices.data();

	VmaAllocationCreateInfo vma_alloc_info = {0};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VK_CHECK(vmaCreateBuffer(graphics_device->vma_allocator,
				 &buffer_create_info,
				 &vma_alloc_info,
				 &buffer.handle,
				 &buffer.allocation,
				 &buffer.allocation_info),
		 "Failed to create buffer.");

	if (GPUBufferIsStorage(&buffer)) {
		VkBufferDeviceAddressInfo address_info = {0};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer.handle;

		buffer.device_address = vkGetBufferDeviceAddress(graphics_device->device, &address_info);
	}

	return buffer;
}

internal void GPUBufferDestroy(GPUBuffer *buffer)
{
	vmaDestroyBuffer(graphics_device->vma_allocator, buffer->handle, buffer->allocation);
	buffer->handle = VK_NULL_HANDLE;
}

internal void GPUBufferRead(GPUBuffer *buffer, void *dst, u64 length, u64 offset)
{
	vmaCopyAllocationToMemory(graphics_device->vma_allocator, buffer->allocation, offset, dst, length);
}

internal void GPUBufferWrite(GPUBuffer *buffer, void *src, u64 length, u64 offset)
{
	vmaCopyMemoryToAllocation(graphics_device->vma_allocator, src, buffer->allocation, offset, length);
}

internal void *GPUBufferData(GPUBuffer *buffer)
{
	return buffer->allocation_info.pMappedData;
}

internal VkBufferMemoryBarrier2 GPUBufferGetMemoryBarrier(GPUBuffer *buffer,
							  GPUBufferAccessInfo src_access_info,
							  GPUBufferAccessInfo dst_access_info)
{
	VkBufferMemoryBarrier2 barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

	barrier.srcAccessMask = src_access_info.access;
	barrier.dstAccessMask = dst_access_info.access;

	barrier.srcStageMask = src_access_info.stage;
	barrier.dstStageMask = dst_access_info.stage;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	
	barrier.buffer = buffer->handle;
	barrier.offset = 0;
	barrier.size = VK_WHOLE_SIZE;
	
	return barrier;
}
