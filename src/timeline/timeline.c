
internal void
TL_Init(TL_Timeline *timeline)
{
	MemZeroStruct(timeline);
}

internal void
TL_Start(TL_Timeline *timeline)
{
	timeline->playing = true;
}

internal void
TL_Stop(TL_Timeline *timeline)
{
	timeline->playing = false;
}

internal void
TL_Tick(TL_Timeline *timeline, void *state, f32 dt)
{
	if (!timeline->playing)
		return;

	b32 all_fired = true;
	
	for (u32 i = 0; i < timeline->event_count; i++)
	{
		TL_Event *ev = &timeline->events[i];

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

internal void
TL_Add(TL_Timeline *timeline,
					 TL_TriggerFn *Trigger,
					 TL_ActionFn *Action,
					 void *data)
{
	AssertTrue(timeline->event_count < ArraySize(timeline->events));
	
	TL_Event *ev = &timeline->events[timeline->event_count];
	ev->Trigger = Trigger;
	ev->Action = Action;
	ev->repeatable = false;

	timeline->event_count++;
}

internal void
TL_AddRepeatable(TL_Timeline *timeline,
							   TL_TriggerFn *Trigger,
							   TL_ActionFn *Action,
							   void *data,
							   f32 cooldown)
{
	AssertTrue(timeline->event_count < ArraySize(timeline->events));
	
	TL_Event *ev = &timeline->events[timeline->event_count];
	ev->Trigger = Trigger;
	ev->Action = Action;
	ev->repeatable = true;
	ev->cooldown = cooldown;

	timeline->event_count++;
}

internal void
TL_Reset(TL_Timeline *timeline)
{
	timeline->finished = false;
	timeline->elapsed = 0.f;
}


/* ==================================================
   UTILITIES
   ================================================== */

internal b32
TL_TriggerAtTime(void *state, f32 elapsed, void *data)
{
	TL_TriggerAtTimeData *t = data;
	return elapsed >= t->timestamp;
}
