#ifndef CHRONO_TIMELINE_H
#define CHRONO_TIMELINE_H

#define CH_TIMELINE_MAX_EVENTS 256

typedef b32 CH_TimelineTriggerFn(void *state, f32 elapsed, void *data);
typedef void CH_TimelineActionFn(void *state, void *data);

typedef struct CH_TimelineEvent CH_TimelineEvent;
struct CH_TimelineEvent
{
	void *data;

	CH_TimelineTriggerFn *Trigger;
	CH_TimelineActionFn *Action;

	b32 fired;
	b32 repeatable;

	f32 cooldown;
	f32 cooldown_remaining;
};

typedef struct CH_Timeline CH_Timeline;
struct CH_Timeline
{
	CH_TimelineEvent events[CH_TIMELINE_MAX_EVENTS];
	u32 event_count;
	
	f32 elapsed;

	b32 playing;
	b32 finished;
};

static void TL_Init  (CH_Timeline *timeline);
static void TL_Start (CH_Timeline *timeline);
static void TL_Stop  (CH_Timeline *timeline);
static void TL_Tick  (CH_Timeline *timeline, void *state, f32 dt);

static void TL_Add(CH_Timeline *timeline,
					 CH_TimelineTriggerFn *Trigger,
					 CH_TimelineActionFn *Action,
					 void *data);

static void CH_TimelineAddRepeatable(CH_Timeline *timeline,
									   CH_TimelineTriggerFn *Trigger,
									   CH_TimelineActionFn *Action,
									   void *data,
									   f32 cooldown);

static void CH_TimelineReset(CH_Timeline *timeline);


/* ==================================================
   UTILITIES
   ================================================== */

typedef struct CH_TimelineTriggerAtTimeData CH_TimelineTriggerAtTimeData;
struct CH_TimelineTriggerAtTimeData
{
	f32 timestamp;
};

static b32 CH_TimelineTriggerAtTime(void *state, f32 elapsed, void *data);


#endif // CHRONO_TIMELINE_H
