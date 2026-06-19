
internal G_TimelinePoint
G_SemaphoreSignal(G_Semaphore *semaphore)
{
	semaphore->target++;

	G_TimelinePoint p = { semaphore->vk_handle, semaphore->target };
	return p;
}

internal G_TimelinePoint
G_SemaphoreLastSignaled(const G_Semaphore *semaphore)
{
	G_TimelinePoint p = { semaphore->vk_handle, semaphore->target };
	return p;
}
