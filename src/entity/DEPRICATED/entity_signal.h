
typedef void (*entity_signal_listener)(void *entity, struct entity_world *world, void *context);

#define ENTITY_SIGNAL_MAX_LISTENERS 16

struct entity_signal {
	struct entity_handle receivers[ENTITY_SIGNAL_MAX_LISTENERS];
	entity_signal_listener listeners[ENTITY_SIGNAL_MAX_LISTENERS];
	u64 count;
};

void entity_signal_connect(struct entity_signal *signal, struct entity_handle receiver, entity_signal_listener listener);
void entity_signal_emit(struct entity_signal *signal, struct entity_world *world, void *context);
void entity_signal_clear(struct entity_signal *signal);

#define ENTITY_SIGNAL_DISPATCHER_MAX_CONTEXT_SIZE 64
#define ENTITY_SIGNAL_DISPATCHER_MAX_DISPATCHED 64

struct entity_signal_dispatcher {
	struct memory_arena scope;

	struct {
		struct entity_signal *signal;
		u8 context[ENTITY_SIGNAL_DISPATCHER_MAX_CONTEXT_SIZE];
	} data[ENTITY_SIGNAL_DISPATCHER_MAX_DISPATCHED];
	
	u32 head;
	u32 tail;
};

void entity_signal_dispatcher_init(struct entity_signal_dispatcher *dispatcher, struct memory_arena *base, u64 memory_allocated);
void entity_signal_dispatcher_enqueue(struct entity_signal_dispatcher *dispatcher, struct entity_signal *signal, void *context);
void entity_signal_dispatcher_dispatch(struct entity_signal_dispatcher *dispatcher, struct entity_world *world);

#if 0

// This adds a signal to dispatch to the dispatcher.
// At the end of the frame, we call all of the callbacks associated with the signal
// (i.e: what it is connected to), per which we give it the entity associated with it.
// Additionally, we pass in the my_context data as extra context.

void entity_my_entity_signal_my_signal(void *entity, struct entity_world *world, void *context);

// Init.
entity_signal_connect(&my_signal, entity_get_handle(my_entity), entity_my_entity_signal_my_signal);

// Update.
entity_signal_dispatcher_enqueue(&world->dispatcher, &my_signal, &my_context);

#endif
