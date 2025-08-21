
typedef struct Timer
{
	b32 started;
	b32 paused;
	
	u64 start_ticks;
	u64 paused_ticks;
}
Timer;
