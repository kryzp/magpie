#include "timer.h"

#include "platform/platform.h"

Timer::Timer()
	: start_ticks(0)
	, started(platform::get_performance_counter())
	, paused_ticks(0)
	, paused(false)
{
}

void Timer::start()
{
	started = true;
	paused = false;

	start_ticks = platform::get_performance_counter();
	paused_ticks = 0;
}

void Timer::stop()
{
	started = false;
	paused = false;

	start_ticks = 0;
	paused_ticks = 0;
}

void Timer::pause()
{
	if (!started || paused)
		return;

	paused = true;
	paused_ticks = platform::get_performance_counter() - start_ticks;

	start_ticks = 0;
}

void Timer::resume()
{
	if (!started || !paused)
		return;

	paused = false;
	start_ticks = platform::get_performance_counter() - paused_ticks;

	paused_ticks = 0;
}

double Timer::reset()
{
	double seconds = get_elapsed_seconds();

	if (started)
		start();

	return seconds;
}

double Timer::get_elapsed_seconds() const
{
	if (started)
	{
		if (paused)
			return (double)paused_ticks / (double)platform::get_performance_frequency();

		return (double)(platform::get_performance_counter() - start_ticks) / (double)platform::get_performance_frequency();
	}

	return 0.0;
}

bool Timer::is_started() const
{
	return started;
}

bool Timer::is_paused() const
{
	return paused;
}
