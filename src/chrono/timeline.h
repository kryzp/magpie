#ifndef CHRONO_TIMELINE_H
#define CHRONO_TIMELINE_H

#include "core/types.h"

#define TIMELINE_MAX_EVENTS 256

typedef bool TimelineTriggerFn(void *state, float elapsed, void *data);
typedef void TimelineActionFn(void *state, void *data);

struct TimelineEvent {
	void *data = nullptr;

	TimelineTriggerFn *trigger = nullptr;
	TimelineActionFn *action = nullptr;

	bool fired = false;
	bool repeatable = false;

	float cooldown = 0.f;
	float cooldown_remaining = 0.f;
};

class Timeline {
public:
	Timeline();

	void Start();
	void Stop();
	void Tick(void *state, float dt);

	void Add(TimelineTriggerFn *trigger, TimelineActionFn *action, void *data);
	void AddRepeatable(TimelineTriggerFn *trigger, TimelineActionFn *action, void *data, float cooldown);

	void Reset();

	bool IsPlaying() const
	{
		return playing;
	}

	bool IsFinished() const
	{
		return finished;
	}

	float GetElapsed() const
	{
		return elapsed;
	}

	struct TriggerAtTimeData {
		float timestamp;
	};

	static bool TriggerAtTime(void *state, float elapsed, void *data);

private:
	TimelineEvent events[TIMELINE_MAX_EVENTS];
	unsigned event_count;

	float elapsed;

	bool playing;
	bool finished;
};

#endif // CHRONO_TIMELINE_H
