#include "chrono/timer.h"
#include "os/platform.h"

Timer::Timer()
	: started(false)
	, paused(false)
	, start_ticks(0)
	, paused_ticks(0)
{
}

void Timer::Start()
{
	started = true;
	paused = false;

	start_ticks = os::GetPerformanceCounter();
	paused_ticks = 0;
}

void Timer::Stop()
{
	started = false;
	paused = false;

	start_ticks = 0;
	paused_ticks = 0;
}

void Timer::Pause()
{
	if (!started || paused)
		return;

	paused = true;
	paused_ticks = os::GetPerformanceCounter() - start_ticks;

	start_ticks = 0;
}

void Timer::Resume()
{
	if (!started || !paused)
		return;

	paused = false;
	start_ticks = os::GetPerformanceCounter() - paused_ticks;

	paused_ticks = 0;
}

double Timer::Restart()
{
	double seconds = Elapsed();
	Start();
	return seconds;
}

double Timer::Elapsed() const
{
	if (!started)
		return 0.0;

	if (paused)
		return static_cast<double>(paused_ticks) / static_cast<double>(os::GetPerformanceFrequency());

	return static_cast<double>(os::GetPerformanceCounter() - start_ticks) / static_cast<double>(os::GetPerformanceFrequency());
}
