#ifndef GRAPHICS_SEMAPHORE_H
#define GRAPHICS_SEMAPHORE_H

#include "core/types.h"

#include <volk/volk.h>

/*
 * We just use timeline semaphores for all synchronisation because it's just
 * a much easier and nicer system in literally every way.
 *
 * They're great!
 */

namespace gfx {
struct TimelinePoint {
	VkSemaphore semaphore = VK_NULL_HANDLE;
	u64 frame = 0;
};

struct Semaphore {
	VkSemaphore vk_handle = VK_NULL_HANDLE;
	u64 last_submitted_frame = 0;

	TimelinePoint Signal()
	{
		last_submitted_frame++;
		return LastSignaled();
	}

	TimelinePoint LastSignaled() const
	{
		TimelinePoint p;
		p.semaphore = vk_handle;
		p.frame = last_submitted_frame;
		return p;
	}
};
}

#endif // GRAPHICS_SEMAPHORE_H
