
internal void
ENT_EventQueueInit(ENT_EventQueue *q)
{
	MemZeroStruct(q);
}

internal void
ENT_EventPush(ENT_EventQueue *q, const ENT_Event *ev)
{
	q->events[q->event_count] = *ev;
	q->event_count++;

	AssertTrue(q->event_count < ArraySize(q->events));
}

internal u64
ENT_EventListenerRegister(ENT_EventQueue *q)
{
	return q->next_listener_id++;
}

internal void
ENT_EventBind(ENT_EventQueue *q,
			  u64 listener_id,
			  ENT_Type entity_type,
			  ENT_EventType event_type,
			  ENT_EventHandlerFn *Handler,
			  void *ctx)
{
	ENT_EventBinding binding = {0};
	binding.listener_id = listener_id;
	binding.entity_type = entity_type;
	binding.event_type = event_type;
	binding.Handler = Handler;
	binding.ctx = ctx;
	
	q->bindings[q->binding_count] = binding;
	q->binding_count++;

	AssertTrue(q->binding_count < ArraySize(q->bindings));	
}

internal void
ENT_EventUnbindAll(ENT_EventQueue *q, u64 listener_id)
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

internal void
ENT_EventDispatch(ENT_EventQueue *q, ENT_World *world)
{
	for (u32 i = 0; i < q->event_count; i++)
	{
		ENT_Event *ev = &q->events[i];

		if (!ENT_UIDIsNull(ev->target))
		{
			void *entity = ENT_WorldGet(world, ev->target);

			if (!entity)
				continue;

			ENT_EventSignal(q, ev, entity);
		}
		else
		{
			ENT_EventBroadcast(q, ev, world);
		}
	}
	
	q->event_count = 0;
}

internal void
ENT_EventSignal(ENT_EventQueue *q, ENT_Event *event, void *entity)
{
	ENT_Header *header = ENT_HeaderOf(entity);

	for (u32 i = 0; i < q->binding_count; i++)
	{
		ENT_EventBinding *b = &q->bindings[i];

		if (b->entity_type == header->type &&
			b->event_type == event->type)
		{
			b->Handler(entity, event, b->ctx);
			break;
		}
	}
}

internal void
ENT_EventBroadcast(ENT_EventQueue *q, ENT_Event *event, ENT_World *world)
{
	for (u32 i = 0; i < q->binding_count; i++)
	{
		ENT_EventBinding *b = &q->bindings[i];

		if (b->event_type != event->type)
			continue;

		ENT_TypePool *pool = &world->pools[b->event_type];
		const ENT_TypeDesc *desc = &ent_global_types[b->event_type];
		
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
