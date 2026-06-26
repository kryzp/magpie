
static void E_EventQueueInit(E_EventQueue *q, LOG_Channel log_channel)
{
	MemZeroStruct(q);

	q->log_channel = log_channel;

	DebugLogI(q->log_channel, "Initialized.");
}

static void E_EventPush(E_EventQueue *q, const E_Event *ev)
{
	DebugLogAssert(q->log_channel,
				   q->event_count < ArraySize(q->events),
				   "Exceeded max number of events (%u).",
				   ArraySize(q->events));

	q->events[q->event_count] = *ev;
	q->event_count++;
}

static u64 E_EventListenerRegister(E_EventQueue *q)
{
	return q->next_listener_id++;
}

static void E_EventBind(E_EventQueue *q,
						u64 listener_id,
						E_TID entity_type,
						E_EventType event_type,
						E_EventHandlerFn *Handler,
						void *ctx)
{
	E_EventBinding binding = {0};
	binding.listener_id = listener_id;
	binding.entity_type = entity_type;
	binding.event_type = event_type;
	binding.Handler = Handler;
	binding.ctx = ctx;
	
	q->bindings[q->binding_count] = binding;
	q->binding_count++;

	AssertTrue(q->binding_count < ArraySize(q->bindings));	
}

static void E_EventUnbindAll(E_EventQueue *q, u64 listener_id)
{
	u32 cursor = 0;
	
	// Since we can't "delete" from the array
	// we can just rewrite over all of the
	// bindings.
	for (u32 b = 0; b < q->binding_count; b++)
	{
		if (q->bindings[b].listener_id != listener_id)
		{
			q->bindings[cursor] = q->bindings[b];
			cursor++;
		}
	}
	
	q->binding_count = cursor;
}

static void E_EventDispatch(E_EventQueue *q, E_World *world)
{
	for (u32 i = 0; i < q->event_count; i++)
	{
		E_Event *ev = &q->events[i];

		if (!E_WorldHandleIsValid(world, ev->target))
		{
			void *entity = E_WorldGet(world, ev->target);

			if (!entity)
				continue;

			E_EventSignal(q, ev, entity);
		}
		else
		{
			E_EventBroadcast(q, ev, world);
		}
	}
	
	q->event_count = 0;
}

static void E_EventSignal(E_EventQueue *q, E_Event *event, void *entity)
{
	E_Header *header = E_HeaderOf(entity);

	for (u32 i = 0; i < q->binding_count; i++)
	{
		E_EventBinding *b = &q->bindings[i];

		if (E_TIDMatch(b->entity_type, header->tid) &&
			b->event_type == event->type)
		{
			b->Handler(entity, event, b->ctx);
			break;
		}
	}
}

static void E_EventBroadcast(E_EventQueue *q, E_Event *event, E_World *world)
{
	for (u32 i = 0; i < q->binding_count; i++)
	{
		E_EventBinding *b = &q->bindings[i];

		if (b->event_type != event->type)
			continue;

		E_TypePool *pool = &world->type_pools[b->event_type];
		const E_TypeDesc *desc = &world->type_registry[b->event_type];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			b->Handler(entity, event, b->ctx);
		}
	}
}
