#ifndef TIMER_H
#define TIMER_H

#include "core/core_types.h"

struct timer {
	bool started;
	bool paused;
	u64 start_ticks;
	u64 paused_ticks;
};

void timer_start(struct timer *timer);
void timer_stop(struct timer *timer);
void timer_pause(struct timer *timer);
void timer_resume(struct timer *timer);
float timer_elapsed_seconds(struct timer *timer);
float timer_reset(struct timer *timer);

#endif // TIMER_H
