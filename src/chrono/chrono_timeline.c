
internal void CH_TimelineInit(CH_Timeline *timeline)
{
	MemZeroStruct(timeline);
}

internal void CH_TimelineStart(CH_Timeline *timeline)
{
	timeline->playing = true;
}

internal void CH_TimelineStop(CH_Timeline *timeline)
{
	timeline->playing = false;
}

internal void CH_TimelineTick(CH_Timeline *timeline, void *state, f32 dt)
{
	if (!timeline->playing)
		return;

	b32 all_fired = true;
	
	for (u32 i = 0; i < timeline->event_count; i++)
	{
		CH_TimelineEvent *ev = &timeline->events[i];

		if (ev->fired && !ev->repeatable)
			continue;
	
		ev->cooldown_remaining -= dt;

		if (ev->cooldown_remaining > 0.f)
			continue;

		all_fired = false;
		
		b32 trigger = ev->Trigger(state, timeline->elapsed, ev->data);

		if (trigger)
		{
			ev->Action(state, ev->data);
			ev->fired = true;
			ev->cooldown_remaining = ev->cooldown;
		}
	}

	if (all_fired)
	{
		timeline->playing = false;
		timeline->finished = true;
	}
	
	timeline->elapsed += dt;
}

internal void CH_TimelineAdd(CH_Timeline *timeline,
			   CH_TimelineTriggerFn *Trigger,
			   CH_TimelineActionFn *Action,
			   void *data)
{
	AssertTrue(timeline->event_count < ArraySize(timeline->events));
	
	CH_TimelineEvent *ev = &timeline->events[timeline->event_count];
	ev->Trigger = Trigger;
	ev->Action = Action;
	ev->repeatable = false;

	timeline->event_count++;
}

internal void CH_TimelineAddRepeatable(CH_Timeline *timeline,
						 CH_TimelineTriggerFn *Trigger,
						 CH_TimelineActionFn *Action,
						 void *data,
						 f32 cooldown)
{
	AssertTrue(timeline->event_count < ArraySize(timeline->events));
	
	CH_TimelineEvent *ev = &timeline->events[timeline->event_count];
	ev->Trigger = Trigger;
	ev->Action = Action;
	ev->repeatable = true;
	ev->cooldown = cooldown;

	timeline->event_count++;
}

internal void CH_TimelineReset(CH_Timeline *timeline)
{
	timeline->finished = false;
	timeline->elapsed = 0.f;
}


/* ==================================================
   UTILITIES
   ================================================== */

internal b32 CH_TimelineTriggerAtTime(void *state, f32 elapsed, void *data)
{
	CH_TimelineTriggerAtTimeData *t = data;
	return elapsed >= t->timestamp;
}
