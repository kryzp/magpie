#ifndef GRAPHICS_SEMAPHORE_H
#define GRAPHICS_SEMAPHORE_H

/*
 * We just use timeline semaphores for all synchronisation because it's just
 * a much easier and nicer system in literally every way.
 *
 * They're great!
 */

typedef struct G_TimelinePoint G_TimelinePoint;
struct G_TimelinePoint
{
	VkSemaphore semaphore;
	u64 frame;
};

typedef struct G_Semaphore G_Semaphore;
struct G_Semaphore
{
	VkSemaphore vk_handle;
	u64 last_submitted_frame;
};

static G_TimelinePoint G_SemaphoreSignal(G_Semaphore *semaphore);
static G_TimelinePoint G_SemaphoreLastSignaled(const G_Semaphore *semaphore);

#endif // GRAPHICS_SEMAPHORE_H
