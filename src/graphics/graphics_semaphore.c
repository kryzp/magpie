
static G_TimelinePoint G_SemaphoreSignal(G_Semaphore *semaphore)
{
	semaphore->last_submitted_frame++;

	G_TimelinePoint p = { semaphore->vk_handle, semaphore->last_submitted_frame };
	return p;
}

static G_TimelinePoint G_SemaphoreLastSignaled(const G_Semaphore *semaphore)
{
	G_TimelinePoint p = { semaphore->vk_handle, semaphore->last_submitted_frame };
	return p;
}
