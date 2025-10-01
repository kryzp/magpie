#ifndef GFX_COMMAND_POOL_H
#define GFX_COMMAND_POOL_H

#include <volk/volk.h>

#include "core/core_math.h"

#include "command_buffer.h"

#define GFX_COMMAND_POOL_MAX_BUFFERS 64

struct gfx_command_pool {
	VkCommandPool handle;
	u32 free_index;
	VkCommandBuffer free_buffers[GFX_COMMAND_POOL_MAX_BUFFERS];
};

struct gfx_command_buffer gfx_command_pool_fetch_free(struct gfx_command_pool *pool);

#endif // GFX_COMMAND_POOL_H
