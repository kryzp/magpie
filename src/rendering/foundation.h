#ifndef GFX_FOUNDATION_H
#define GFX_FOUNDATION_H

#include "device.h"

/* 
 * TODO: This hasn't been integrated yet or anything but the idea is that the device does a lot more
 *       than it really should be doing, we can package everything together into a final "gfx_foundation"
 *       that can encapsulates the entire graphics backend. Currently, we need to create a swapchain
 *       seperately for instance, because the device itself shouldn't own the swapchain.
 */

struct gfx_foundation {
	struct memory_arena *arena;
	struct gfx_device device;
	struct gfx_queue graphics_queue;
	struct gfx_swapchain swapchain;
	struct gfx_bindless bindless;
	struct gfx_resources resources;
	struct hash_table image_view_cache;
	struct hash_table pipeline_cache;
	struct hash_table pipeline_layout_cache;
};

void gfx_foundation_init(struct gfx_foundation *foundation, struct memory_arena *arena);
void gfx_foundation_destroy(struct gfx_foundation *foundation);

#endif // GFX_FOUNDATION_H
