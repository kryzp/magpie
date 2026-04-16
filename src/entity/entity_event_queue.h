#ifndef ENTITY_EVENT_QUEUE_H
#define ENTITY_EVENT_QUEUE_H

// TODO: rename this its more of an
//       event manager atp

typedef struct ENT_World ENT_World;

#define ENT_EVENT_QUEUE_MAX_PER_FRAME  512
#define ENT_EVENT_QUEUE_MAX_BINDINGS   512

typedef void ENT_EventHandlerFn(void *entity, const ENT_Event *event);

// listeners
typedef struct ENT_EventBinding ENT_EventBinding;
struct ENT_EventBinding
{
	ENT_Type entity_type;
	ENT_EventType event_type;
	ENT_EventHandlerFn *Handler;
};

typedef struct ENT_EventQueue ENT_EventQueue;
struct ENT_EventQueue
{
	ENT_Event events[ENT_EVENT_QUEUE_MAX_PER_FRAME];
	u32 event_count;

	ENT_EventBinding bindings[ENT_EVENT_QUEUE_MAX_BINDINGS];
	u32 binding_count;
};

internal void ENT_EventQueueInit(ENT_EventQueue *q);

internal void ENT_EventPush(ENT_EventQueue *q, const ENT_Event *ev);

internal void ENT_EventBind(ENT_EventQueue *q,
							ENT_Type entity_type,
							ENT_EventType event_type,
							ENT_EventHandlerFn *Handler);

internal void ENT_EventDispatch(ENT_EventQueue *q, ENT_World *world);

internal void ENT_EventSignal    (ENT_EventQueue *q, ENT_Event *event, void *entity);
internal void ENT_EventBroadcast (ENT_EventQueue *q, ENT_Event *event, ENT_World *world);

// ---

internal inline void
ENT_EventFireSomeRandomThing(ENT_EventQueue *q,
							 ENT_UID source, ENT_UID target,
							 f32 random_data)
{
	ENT_Event ev = {0};
	ev.type = ENT_EventType_SomeRandomThing;
	ev.source = source;
	ev.target = target;
	ev.timestamp = 0.f; // todo - OS call? or time relative to game world (because maybe time slowdown)?
	ev.some_random_thing.random_data = random_data;
	
	ENT_EventPush(q, &ev);
}

#endif // ENTITY_EVENT_QUEUE_H
