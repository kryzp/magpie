#ifndef GRAPHICS_CMD_POOL
#define GRAPHICS_CMD_POOL

#define GFX_CMD_POOL_MAX_BUFFERS 32

typedef struct GFX_CmdPool GFX_CmdPool;
struct GFX_CmdPool
{
	VkCommandPool handle;
	u32 used_count;
	VkCommandBuffer buffers[GFX_CMD_POOL_MAX_BUFFERS];
};

#endif // GRAPHICS_CMD_POOL
