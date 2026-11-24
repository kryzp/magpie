#ifndef CHRONO_TIMER_H
#define CHRONO_TIMER_H

#include "core/types.h"

class Timer {
public:
	Timer();

	void Start();
	void Stop();

	void Pause();
	void Resume();

	double Restart();
	double Elapsed() const;

	bool Started() const
	{
		return started;
	}

	bool Paused() const
	{
		return paused;
	}

private:
	bool started;
	bool paused;

	u64 start_ticks;
	u64 paused_ticks;
};

#endif // CHRONO_TIMER_H
