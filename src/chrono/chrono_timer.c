
void CH_TimerStart(CH_Timer *timer)
{
	timer->started = true;
	timer->paused = false;

	timer->start_ticks = osapi->GetPerformanceCounter();
	timer->paused_ticks = 0;
}

void CH_TimerStop(CH_Timer *timer)
{
	timer->started = false;
	timer->paused = false;

	timer->start_ticks = 0;
	timer->paused_ticks = 0;
}

void CH_TimerPause(CH_Timer *timer)
{
	if (!timer->started || timer->paused)
		return;

	timer->paused = true;
	timer->paused_ticks = osapi->GetPerformanceCounter() - timer->start_ticks;

	timer->start_ticks = 0;
}

void CH_TimerResume(CH_Timer *timer)
{
	if (!timer->started || !timer->paused)
		return;

	timer->paused = false;
	timer->start_ticks = osapi->GetPerformanceCounter() - timer->paused_ticks;

	timer->paused_ticks = 0;
}

f64 CH_TimerReset(CH_Timer *timer)
{
	f64 seconds = CH_TimerElapsed(timer);

	if (timer->started)
		CH_TimerStart(timer);

	return seconds;
}

f64 CH_TimerElapsed(const CH_Timer *timer)
{
	if (timer->started)
	{
		if (timer->paused)
		{
			return (f64)timer->paused_ticks / (f64)osapi->GetPerformanceFrequency();
		}
		else
		{
			return (f64)(osapi->GetPerformanceCounter() - timer->start_ticks) / (f64)osapi->GetPerformanceFrequency();
		}
	}
	else
	{
		return 0.0;
	}
}

b32 CH_TimerStarted(const CH_Timer *timer)
{
	return timer->started;
}

b32 CH_TimerPaused(const CH_Timer *timer)
{
	return timer->paused;
}
