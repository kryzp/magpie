#ifndef CHRONO_TIMER_H
#define CHRONO_TIMER_H

typedef struct CH_Timer CH_Timer;
struct CH_Timer
{
	b32 started;
	b32 paused;
	
	u64 start_ticks;
	u64 paused_ticks;
};

void CH_TimerStart(CH_Timer *timer);
void CH_TimerStop(CH_Timer *timer);

void CH_TimerPause(CH_Timer *timer);
void CH_TimerResume(CH_Timer *timer);

f64 CH_TimerRestart(CH_Timer *timer);
f64 CH_TimerElapsed(const CH_Timer *timer);

b32 CH_TimerStarted(const CH_Timer *timer);
b32 CH_TimerPaused(const CH_Timer *timer);

#endif // CHRONO_TIMER_H
