#include "graphics/cmd_pool.h"
#include "graphics/cmd_buffer.h"
#include "graphics/vk_check.h"

gfx::CmdPool::CmdPool(VkDevice device_, unsigned family_index, LogChannel log_channel_)
	: device(device_)
	, vk_handle(VK_NULL_HANDLE)
	, log_channel(log_channel_)
	, acquire_count(0)
	, release_front(0)
	, release_count(0)
{
	VkCommandPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;

	GRAPHICS_VK_CHECK(
		vkCreateCommandPool(device, &create_info, nullptr, &vk_handle),
		"Failed to create command pool."
	);
}

gfx::CmdPool::~CmdPool()
{
	vkDestroyCommandPool(device, vk_handle, nullptr);
}

gfx::CmdBuffer gfx::CmdPool::Acquire()
{
	VkCommandBuffer cb = VK_NULL_HANDLE;

	if (acquire_count > 0) {
		cb = acquire_stack[--acquire_count];

		vkResetCommandBuffer(cb, 0);
	} else {
		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;
		alloc_info.commandPool = vk_handle;

		GRAPHICS_VK_CHECK(
			vkAllocateCommandBuffers(device, &alloc_info, &cb),
			"Failed to allocate command pool command buffers."
		);
	}

	return CmdBuffer(cb);
}

void gfx::CmdPool::Release(const CmdBuffer &cmd, u64 fence_value)
{
	DebugLogAssert(
		log_channel,
		release_count < (int)ArraySize(release_queue),
		"Command pool release queue is full."
	);

	unsigned slot = (unsigned)(release_front + release_count) % ArraySize(release_queue);

	CmdPoolReleasedBuffer *released = &release_queue[slot];

	released->vk_handle = cmd.GetVkHandle();
	released->fence_value = fence_value;

	release_count++;
}

void gfx::CmdPool::Purge(u64 fence_value)
{
	while (release_count > 0) {
		CmdPoolReleasedBuffer *released = &release_queue[release_front];

		if (released->fence_value > fence_value)
			break;

		DebugLogAssert(
			log_channel,
			acquire_count < ArraySize(acquire_stack),
			"Command pool acquire stack is full."
		);

		acquire_stack[acquire_count++] = released->vk_handle;

		release_front = (release_front + 1) % (int)ArraySize(release_queue);

		release_count--;
	}
}
