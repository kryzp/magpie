#ifndef GRAPHICS_CMD_POOL
#define GRAPHICS_CMD_POOL

#define GFX_CMD_POOL_MAX_BUFFERS 512

typedef struct GFX_CmdPoolReleasedBuffer GFX_CmdPoolReleasedBuffer;
struct GFX_CmdPoolReleasedBuffer
{
	VkCommandBuffer vk_handle;
	u64 fence_value;
};

typedef struct GFX_CmdPool GFX_CmdPool;
struct GFX_CmdPool
{
	VkCommandPool vk_handle;

	u32 acquire_count;
	VkCommandBuffer acquire_stack[GFX_CMD_POOL_MAX_BUFFERS];

	i32 release_front;
	i32 release_count;
	GFX_CmdPoolReleasedBuffer release_queue[GFX_CMD_POOL_MAX_BUFFERS];
};

#endif // GRAPHICS_CMD_POOL
