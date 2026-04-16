#ifndef TIMELINE_H
#define TIMELINE_H

#define TIMELINE_MAX_EVENTS 256

typedef b32 TL_TriggerFn(void *state, f32 elapsed, void *data);
typedef void TL_ActionFn(void *state, void *data);

typedef struct TL_Event TL_Event;
struct TL_Event
{
	void *data;

	TL_TriggerFn *Trigger;
	TL_ActionFn *Action;

	b32 fired;
	b32 repeatable;
};

typedef struct TL_Timeline TL_Timeline;
struct TL_Timeline
{
	TL_Event events[TIMELINE_MAX_EVENTS];
	u32 event_count;
	
	f32 elapsed;

	b32 playing;
	b32 finished;
};

internal void TL_Init  (TL_Timeline *timeline);
internal void TL_Start (TL_Timeline *timeline);
internal void TL_Stop  (TL_Timeline *timeline);
internal void TL_Tick  (TL_Timeline *timeline, void *state, f32 dt);

internal void TL_Add(TL_Timeline *timeline,
					 TL_TriggerFn *Trigger,
					 TL_ActionFn *Action,
					 void *data, b32 repeatable);

internal void TL_Reset(TL_Timeline *timeline);


/* ==================================================
   UTILITIES
   ================================================== */

typedef struct TL_TriggerAtTimeData TL_TriggerAtTimeData;
struct TL_TriggerAtTimeData
{
	f32 timestamp;
};

internal b32 TL_TriggerAtTime(void *state, f32 elapsed, void *data);


#endif // TIMELINE_H
