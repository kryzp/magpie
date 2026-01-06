
void entity_signal_connect(struct entity_signal *signal, struct entity_handle receiver, entity_signal_listener listener)
{
	signal->receivers[signal->count] = receiver;
	signal->listeners[signal->count] = listener;
	
	signal->count++;
}

void entity_signal_emit(struct entity_signal *signal, struct entity_world *world, void *context)
{
	for (u32 i = 0; i < signal->count; i++) {
		void *entity = entity_world_get_entity(world, signal->receivers[i]);
		signal->listeners[i](entity, world, context);
	}
}

void entity_signal_clear(struct entity_signal *signal)
{
	signal->count = 0;
}

void entity_signal_dispatcher_init(struct entity_signal_dispatcher *dispatcher, struct memory_arena *arena, u64 memory_allocated)
{
	dispatcher->scope = memory_arena_sub_arena(arena, memory_allocated);
}

void entity_signal_dispatcher_enqueue(struct entity_signal_dispatcher *dispatcher, struct entity_signal *signal, void *context)
{
	dispatcher->data[dispatcher->tail].signal = signal;

	if (context)
		memory_copy(dispatcher->data[dispatcher->tail].context, (u8 *)context,
			    ENTITY_SIGNAL_DISPATCHER_MAX_CONTEXT_SIZE);
	
	dispatcher->tail = (dispatcher->tail + 1) % ENTITY_SIGNAL_DISPATCHER_MAX_DISPATCHED;
}

void entity_signal_dispatcher_dispatch(struct entity_signal_dispatcher *dispatcher, struct entity_world *world)
{
	for (; dispatcher->head != dispatcher->tail;
	       dispatcher->head = (dispatcher->head + 1) % ENTITY_SIGNAL_DISPATCHER_MAX_DISPATCHED)
		entity_signal_emit(dispatcher->data[dispatcher->head].signal, world,
				   dispatcher->data[dispatcher->head].context);
}
