#ifndef GFX_QUEUE_H
#define GFX_QUEUE_H

#include <volk/volk.h>

#include "core/core_types.h"

struct gfx_queue {
	VkQueue handle;
	u32 family_index;
};

void gfx_queue_wait_idle(struct gfx_queue *queue);
void gfx_queue_submit(struct gfx_queue *queue, const VkSubmitInfo2 *submit_info, VkFence fence);
VkResult gfx_queue_present(struct gfx_queue *queue, const VkPresentInfoKHR *present_info);

#endif // GFX_QUEUE_H
