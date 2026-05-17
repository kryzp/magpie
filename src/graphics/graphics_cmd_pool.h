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

	i32 acquire_queue_front;
	i32 acquire_queue_back;
	VkCommandBuffer acquire_queue[GFX_CMD_POOL_MAX_BUFFERS];

	i32 release_queue_front;
	i32 release_queue_back;
	GFX_CmdPoolReleasedBuffer release_queue[GFX_CMD_POOL_MAX_BUFFERS];
};

internal b32 GFX_CmdPoolHasEmptyAcquireQueue(const GFX_CmdPool *pool);
internal b32 GFX_CmdPoolHasEmptyReleaseQueue(const GFX_CmdPool *pool);

internal b32 GFX_CmdPoolHasFullAcquireQueue(const GFX_CmdPool *pool);
internal b32 GFX_CmdPoolHasFullReleaseQueue(const GFX_CmdPool *pool);

#endif // GRAPHICS_CMD_POOL
