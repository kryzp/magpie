#include "timer.h"
#include "app.h"

void timer_start(struct timer *timer)
{
	timer->started = true;
	timer->paused = false;

	timer->start_ticks = platform->get_performance_counter();
	timer->paused_ticks = 0;
}

void timer_stop(struct timer *timer)
{
	timer->started = false;
	timer->paused = false;

	timer->start_ticks = 0;
	timer->paused_ticks = 0;
}

void timer_pause(struct timer *timer)
{
	if (!timer->started || timer->paused)
		return;

	timer->paused = true;
	timer->paused_ticks = platform->get_performance_counter() - timer->start_ticks;

	timer->start_ticks = 0;
}

void timer_resume(struct timer *timer)
{
	if (!timer->started || !timer->paused)
		return;

	timer->paused = false;
	timer->start_ticks = platform->get_performance_counter() - timer->paused_ticks;

	timer->paused_ticks = 0;
}

float timer_elapsed_seconds(struct timer *timer)
{
	if (timer->started) {
		if (timer->paused)
			return (float)timer->paused_ticks / (float)platform->get_performance_frequency();

		return (float)(platform->get_performance_counter() - timer->start_ticks) / (float)platform->get_performance_frequency();
	}

	return 0.f;
}

float timer_reset(struct timer *timer)
{
	float sec = timer_elapsed_seconds(timer);

	if (timer->started)
		timer_start(timer);
	
	return sec;
}
