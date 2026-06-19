#ifndef GRAPHICS_CMD_POOL
#define GRAPHICS_CMD_POOL

#define G_CMD_POOL_MAX_BUFFERS 512

typedef struct G_CmdPoolReleasedBuffer G_CmdPoolReleasedBuffer;
struct G_CmdPoolReleasedBuffer
{
	VkCommandBuffer vk_handle;
	u64 fence_value;
};

typedef struct G_CmdPool G_CmdPool;
struct G_CmdPool
{
	VkCommandPool vk_handle;

	u32 acquire_count;
	VkCommandBuffer acquire_stack[G_CMD_POOL_MAX_BUFFERS];

	i32 release_front;
	i32 release_count;
	G_CmdPoolReleasedBuffer release_queue[G_CMD_POOL_MAX_BUFFERS];
};

#endif // GRAPHICS_CMD_POOL
