#ifndef ENTITY_EVENT_QUEUE_H
#define ENTITY_EVENT_QUEUE_H

// TODO: rename this its more of an
//       event manager atp

typedef struct ENT_World ENT_World;

#define ENT_EVENT_QUEUE_MAX_PER_FRAME  512
#define ENT_EVENT_QUEUE_MAX_BINDINGS   512

typedef void ENT_EventHandlerFn(void *entity, const ENT_Event *event, void *ctx);

typedef struct ENT_EventBinding ENT_EventBinding;
struct ENT_EventBinding
{
	u64 listener_id;
	ENT_Type entity_type;
	ENT_EventType event_type;
	ENT_EventHandlerFn *Handler;
	void *ctx;
};

typedef struct ENT_EventQueue ENT_EventQueue;
struct ENT_EventQueue
{
	ENT_Event events[ENT_EVENT_QUEUE_MAX_PER_FRAME];
	u32 event_count;

	ENT_EventBinding bindings[ENT_EVENT_QUEUE_MAX_BINDINGS];
	u32 binding_count;
	
	u64 next_listener_id;
};

internal void ENT_EventQueueInit(ENT_EventQueue *q);

internal void ENT_EventPush(ENT_EventQueue *q, const ENT_Event *ev);

internal u64  ENT_EventListenerRegister(ENT_EventQueue *q);

internal void ENT_EventBind(ENT_EventQueue *q,
							u64 listener_id,
							ENT_Type entity_type,
							ENT_EventType event_type,
							ENT_EventHandlerFn *Handler,
							void *ctx);

internal void ENT_EventUnbindAll(ENT_EventQueue *q, u64 listener_id);

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
