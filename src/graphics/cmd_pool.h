#ifndef GRAPHICS_CMD_POOL_H
#define GRAPHICS_CMD_POOL_H

#include "core/types.h"
#include "core/log.h"

#include <volk/volk.h>

#define GRAPHICS_CMD_POOL_MAX_BUFFERS 512

namespace gfx {
class CmdBuffer;

struct CmdPoolReleasedBuffer {
	VkCommandBuffer vk_handle;
	u64 fence_value;
};

// recycle command buffers once the timeline
// valeu they were submitted with
// has been reached.
class CmdPool {
public:
	CmdPool(VkDevice device, unsigned family_index, LogChannel log_channel);
	~CmdPool();

	CmdPool(const CmdPool &) = delete;
	CmdPool &operator=(const CmdPool &) = delete;

	CmdBuffer Acquire();
	void Release(const CmdBuffer &cmd, u64 fence_value);
	void Purge(u64 fence_value);

private:
	VkDevice device;
	VkCommandPool vk_handle;
	LogChannel log_channel;

	unsigned acquire_count;
	VkCommandBuffer acquire_stack[GRAPHICS_CMD_POOL_MAX_BUFFERS];

	int release_front;
	int release_count;
	CmdPoolReleasedBuffer release_queue[GRAPHICS_CMD_POOL_MAX_BUFFERS];
};
}

#endif // GRAPHICS_CMD_POOL_H
