#ifndef GRAPHICS_SEMAPHORE_H
#define GRAPHICS_SEMAPHORE_H

/*
 * We just use timeline semaphores for all synchronisation because it's just
 * a much easier and nicer system in literally every way.
 *
 * They're great!
 */

typedef struct GFX_TimelinePoint GFX_TimelinePoint;
struct GFX_TimelinePoint
{
	VkSemaphore semaphore;
	u64 value;
};

typedef struct GFX_Semaphore GFX_Semaphore;
struct GFX_Semaphore
{
	VkSemaphore vk_handle;
	u64 target;
};

internal GFX_TimelinePoint GFX_SemaphoreSignal(GFX_Semaphore *semaphore);
internal GFX_TimelinePoint GFX_SemaphoreLastSignaled(const GFX_Semaphore *semaphore);

#endif // GRAPHICS_SEMAPHORE_H
