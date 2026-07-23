#ifndef ENTITY_EVENT_QUEUE_H
#define ENTITY_EVENT_QUEUE_H

// TODO: rename this its more of an
//       event manager atp

typedef struct E_World E_World;

#define E_EVE_QUEUE_MAX_PER_FRAME  512
#define E_EVE_QUEUE_MAX_BINDINGS   512

typedef void E_EventHandlerFn(void *entity, const E_Event *event, void *ctx);

typedef struct E_EventBinding E_EventBinding;
struct E_EventBinding
{
	u64 listener_id;
	u32 entity_type;
	E_EventType event_type;
	E_EventHandlerFn *Handler;
	void *ctx;
};

typedef struct E_EventQueue E_EventQueue;
struct E_EventQueue
{
	LOG_Channel log_channel;
	
	E_Event events[E_EVE_QUEUE_MAX_PER_FRAME];
	u32 event_count;

	E_EventBinding bindings[E_EVE_QUEUE_MAX_BINDINGS];
	u32 binding_count;
	
	u64 next_listener_id;
};

internal void E_EventQueueInit(E_EventQueue *q, LOG_Channel log_channel);

internal void E_EventPush(E_EventQueue *q, const E_Event *ev);

internal u64  E_EventListenerRegister(E_EventQueue *q);

internal void E_EventBind(E_EventQueue *q,
						u64 listener_id,
						u32 entity_type,
						E_EventType event_type,
						E_EventHandlerFn *Handler,
						void *ctx);

internal void E_EventUnbindAll(E_EventQueue *q, u64 listener_id);

internal void E_EventDispatch(E_EventQueue *q, E_World *world);

internal void E_EventSignal    (E_EventQueue *q, E_Event *event, void *entity);
internal void E_EventBroadcast (E_EventQueue *q, E_Event *event, E_World *world);

// ---

internal inline void E_EventFireSomeRandomThing(E_EventQueue *q,
							 E_Handle source, E_Handle target,
							 f32 random_data)
{
	E_Event ev = {0};
	ev.type = E_EventType_SomeRandomThing;
	ev.source = source;
	ev.target = target;
	ev.timestamp = 0.f; // todo - OS call? or time relative to game world (because maybe time slowdown)?
	ev.some_random_thing.random_data = random_data;
	
	E_EventPush(q, &ev);
}

#endif // ENTITY_EVENT_QUEUE_H
