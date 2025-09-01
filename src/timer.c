
internal f32 GetTotalElapsedSecondsF()
{
	return (f32)(platform->GetPerformanceCounter() - core->starting_ticks) / (f32)platform->GetPerformanceFrequency();
}

// ---

internal void TimerStart(Timer *timer)
{
	timer->started = true;
	timer->paused = false;

	timer->start_ticks = platform->GetPerformanceCounter();
	timer->paused_ticks = 0;
}

internal void TimerStop(Timer *timer)
{
	timer->started = false;
	timer->paused = false;

	timer->start_ticks = 0;
	timer->paused_ticks = 0;
}

internal void TimerPause(Timer *timer)
{
	if (!timer->started || timer->paused)
		return;

	timer->paused = true;
	timer->paused_ticks = platform->GetPerformanceCounter() - timer->start_ticks;

	timer->start_ticks = 0;
}

internal void TimerResume(Timer *timer)
{
	if (!timer->started || !timer->paused)
		return;

	timer->paused = false;
	timer->start_ticks = platform->GetPerformanceCounter() - timer->paused_ticks;

	timer->paused_ticks = 0;
}

internal f32 TimerGetElapsedSecondsF(Timer *timer)
{
	if (timer->started) {
		if (timer->paused)
			return (f32)timer->paused_ticks / (f32)platform->GetPerformanceFrequency();

		return (f32)(platform->GetPerformanceFrequency() - timer->start_ticks) / (f32)platform->GetPerformanceFrequency();
	}

	return 0.f;
}

internal f32 TimerResetF(Timer *timer)
{
	f32 sec = TimerGetElapsedSecondsF(timer);

	if (timer->started)
		TimerStart(timer);
	
	return sec;
}
