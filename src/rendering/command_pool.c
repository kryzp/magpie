#include "command_pool.h"

struct gfx_command_buffer gfx_command_pool_fetch_free(struct gfx_command_pool *pool)
{
	struct gfx_command_buffer cmd = {0};
	cmd.handle = pool->free_buffers[pool->free_index++];
	
	return cmd;
}
