#pragma once

#include "core/types.h"

class Platform;

class Timer {
public:
	Timer(const Platform &platform);
	~Timer() = default;

	void start();
	void stop();

	void pause();
	void resume();

	double reset();

	double get_elapsed_seconds() const;

	bool is_started() const;
	bool is_paused() const;

private:
	const Platform &platform;

	u64 start_ticks;
	bool started;

	u64 paused_ticks;
	bool paused;
};
