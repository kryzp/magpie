
static void AN_SystemInit(AN_System *s, LOG_Channel log_channel)
{
	s->log_channel = log_channel;

	s->instance_count = 0;
	s->free_index_count = 0;

	for (u32 i = 0; i < ArraySize(s->instances); i++)
	{
		s->instances[i].arena = ArenaAlloc(Megabytes(16));
		s->instances[i].alive = false;
		s->instances[i].generation = 0;
	}
}

static void AN_SystemDestroy(AN_System *s)
{
	for (u32 i = 0; i < ArraySize(s->instances); i++)
	{
		ArenaRelease(&s->instances[i].arena);
	}
}

static void AN_SystemTick(AN_System *s, A_Registry *assets, f32 dt)
{
	for (u32 i = 0; i < s->instance_count; i++)
	{
		AN_Instance *inst = &s->instances[i];

		if (!inst->alive)
			continue;

		AN_AnimatorTick(&inst->animator, assets, dt);
	}
}

static AN_Instance *AN_SystemResolve(AN_System *s, AN_Handle handle)
{
	if (handle.index >= ArraySize(s->instances))
		return NULL;

	AN_Instance *inst = &s->instances[handle.index];

	if (!inst->alive || inst->generation != handle.generation)
		return NULL;

	return inst;
}

static AN_Handle AN_SystemCreateInstance(AN_System *s, A_Registry *assets, A_Handle model_handle)
{
	AN_Handle result = AN_HandleNull();

	u32 index = 0;

	if (s->free_index_count > 0)
	{
		s->free_index_count--;
		index = s->free_indices[s->free_index_count];
	}
	else
	{
		if (s->instance_count >= ArraySize(s->instances))
		{
			DebugLogE(s->log_channel,
					  "Instance pool exhausted (max %u).",
					  ArraySize(s->instances));

			return result;
		}

		index = s->instance_count;

		s->instance_count++;
	}

	AN_Instance *inst = &s->instances[index];

	ArenaReset(&inst->arena);

	AN_AnimatorSelect(&inst->animator, &inst->arena, assets, model_handle);

	inst->alive = true;

	result.index = index;
	result.generation = inst->generation;

	return result;
}

static void AN_SystemKillInstance(AN_System *s, AN_Handle h)
{
	AN_Instance *inst = AN_SystemResolve(s, h);

	if (!inst)
		return;

	inst->alive = false;
	inst->generation++;

	s->free_indices[s->free_index_count] = h.index;
	s->free_index_count++;
}
