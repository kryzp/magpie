
internal GFX_TimelinePoint
GFX_SemaphoreSignal(GFX_Semaphore *semaphore)
{
	semaphore->target++;

	GFX_TimelinePoint p = { semaphore->handle, semaphore->target };
	return p;
}

internal GFX_TimelinePoint
GFX_SemaphoreLastSignaled(const GFX_Semaphore *semaphore)
{
	GFX_TimelinePoint p = { semaphore->handle, semaphore->target };
	return p;
}
