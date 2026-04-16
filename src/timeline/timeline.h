#ifndef TIMELINE_H
#define TIMELINE_H

#define TIMELINE_MAX_EVENTS 256

typedef b32  TL_TriggerFn(void *state, f32 elapsed, void *data);
typedef void TL_ActionFn(void *state, void *data);

typedef struct TL_Event TL_Event;
struct TL_Event
{
	void *data;
	TL_TriggerFn *trigger;
	TL_ActionFn  *action;
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

void TL_Init  (TL_Timeline *timeline);
void TL_Start (TL_Timeline *timeline);
void TL_Stop  (TL_Timeline *timeline);
void TL_Tick  (TL_Timeline *timeline, void *state, f32 dt);

void TL_Add(TL_Timeline *timeline,
			TL_TriggerFn *trigger,
			TL_ActionFn *action,
			void *data);

void TL_Reset(TL_Timeline *timeline);


/* ==================================================
   UTILITIES
   ================================================== */

typedef struct TL_TriggerAtTimeData TL_TriggerAtTimeData;
struct TL_TriggerAtTimeData
{
	f32 timestamp;
};

b32 TL_TriggerAtTime(void *state, f32 elapsed, void *data);


typedef struct TL_TriggerOnEntityInSphereData TL_TriggerOnEntityInSphereData;
struct TL_TriggerOnEntityInSphereData
{
	ENT_UID entity_uid;
	v3 centre;
	f32 radius;
};

b32 TL_TriggerOnEntityInSphere(void *state, f32 elapsed, void *data);


typedef struct TL_TriggerOnEntityDeathData TL_TriggerOnEntityDeathData;
struct TL_TriggerOnEntityDeathData
{
	ENT_UID entity_uid;
};

b32 TL_TriggerOnEntityDeath(void *state, f32 elapsed, void *data);


#endif // TIMELINE_H
