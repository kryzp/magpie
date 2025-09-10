
internal void CommandPoolInit(CommandPool *pool, u32 family_index)
{
	VkCommandPoolCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;

	VK_CHECK(vkCreateCommandPool(graphics_device->device, &create_info, 0, &pool->handle),
		 "Failed to create command pool.");

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {0};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = ArraySize(pool->free_buffers);
	command_buffer_allocate_info.commandPool = pool->handle;

	VK_CHECK(vkAllocateCommandBuffers(graphics_device->device,
					  &command_buffer_allocate_info,
					  pool->free_buffers),
		 "Failed to allocate command pool command buffers.");
}

internal void CommandPoolDestroy(CommandPool *pool)
{
	vkDestroyCommandPool(graphics_device->device, pool->handle, 0);
	pool->handle = VK_NULL_HANDLE;
}

internal CommandBuffer CommandPoolFetchFreeBuffer(CommandPool *pool)
{
	CommandBuffer cmd = {0};
	cmd.handle = pool->free_buffers[pool->free_index++];

	return cmd;
}

internal void CommandPoolReset(CommandPool *pool)
{
	vkResetCommandPool(graphics_device->device, pool->handle, 0);
	pool->free_index = 0;
}
