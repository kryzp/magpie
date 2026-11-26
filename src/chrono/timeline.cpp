#include "chrono/timeline.h"
#include "core/log.h"

Timeline::Timeline()
	: event_count(0)
	, elapsed(0.f)
	, playing(false)
	, finished(false)
{
}

void Timeline::Start()
{
	playing = true;
}

void Timeline::Stop()
{
	playing = false;
}

void Timeline::Tick(void *state, float dt)
{
	if (!playing)
		return;

	bool all_fired = true;

	for (int i = 0; i < event_count; i++) {
		TimelineEvent *ev = &events[i];

		if (ev->fired && !ev->repeatable)
			continue;

		ev->cooldown_remaining -= dt;

		if (ev->cooldown_remaining > 0.f)
			continue;

		all_fired = false;

		bool trigger = ev->trigger(state, elapsed, ev->data);

		if (trigger) {
			ev->action(state, ev->data);
			ev->fired = true;
			ev->cooldown_remaining = ev->cooldown;
		}
	}

	if (all_fired) {
		playing = false;
		finished = true;
	}

	elapsed += dt;
}

void Timeline::Add(TimelineTriggerFn *trigger, TimelineActionFn *action, void *data)
{
	AssertTrue(event_count < ArraySize(events));

	TimelineEvent *ev = &events[event_count];
	ev->trigger = trigger;
	ev->action = action;
	ev->data = data;
	ev->repeatable = false;

	event_count++;
}

void Timeline::AddRepeatable(TimelineTriggerFn *trigger, TimelineActionFn *action, void *data, float cooldown)
{
	AssertTrue(event_count < ArraySize(events));

	TimelineEvent *ev = &events[event_count];
	ev->trigger = trigger;
	ev->action = action;
	ev->data = data;
	ev->repeatable = true;
	ev->cooldown = cooldown;

	event_count++;
}

void Timeline::Reset()
{
	finished = false;
	elapsed = 0.f;
}

bool Timeline::TriggerAtTime(void *state, float elapsed, void *data)
{
	(void)state;

	TriggerAtTimeData *t = static_cast<TriggerAtTimeData *>(data);
	return elapsed >= t->timestamp;
}
