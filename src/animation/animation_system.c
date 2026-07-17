
static AN_System *an_system = NULL;

static void AN_SystemInitAndSelect(AN_System *system, LOG_Channel log_channel)
{
	system->log_channel = log_channel;

	system->instance_count = 0;
	system->free_index_count = 0;

	for (u32 i = 0; i < ArraySize(system->instances); i++)
	{
		system->instances[i].arena = ArenaAlloc(Megabytes(2));
		system->instances[i].alive = false;
		system->instances[i].generation = 0;
	}

	AN_SystemSelectContext(system);

	DebugLogI(system->log_channel, "Initialized.");
}

static void AN_SystemDestroy(void)
{
	for (u32 i = 0; i < ArraySize(an_system->instances); i++)
	{
		ArenaRelease(&an_system->instances[i].arena);
	}

	DebugLogI(an_system->log_channel, "Destroyed.");

	an_system = NULL;
}

static void AN_SystemSelectContext(AN_System *system)
{
	an_system = system;
}

static void AN_SystemCalculateIntermediatePoses(f32 elapsed)
{
	for (u32 i = 0; i < an_system->instance_count; i++)
	{
		AN_Instance *inst = &an_system->instances[i];

		if (!inst->alive)
			continue;

		AN_AnimatorTick(&inst->animator, elapsed);
	}
}

static void AN_SystemFinalizePoseAndMatrixPalette(void)
{
	for (u32 i = 0; i < an_system->instance_count; i++)
	{
		AN_Instance *inst = &an_system->instances[i];

		if (!inst->alive)
			continue;

		AN_AnimatorUpdatePalette(&inst->animator);
	}
}

static AN_Instance *AN_SystemResolve(AN_Handle handle)
{
	if (handle.index >= ArraySize(an_system->instances))
		return NULL;

	AN_Instance *inst = &an_system->instances[handle.index];

	if (!inst->alive || inst->generation != handle.generation)
		return NULL;

	return inst;
}

static AN_Animator *AN_SystemGetAnimator(AN_Handle handle)
{
	AN_Instance *inst = AN_SystemResolve(handle);

	DebugLogAssert(an_system->log_channel, inst, "Handle is null.");

	return &inst->animator;
}

static void AN_Play(AN_Handle handle, AN_ClipKey clip, b32 loop, f32 global_start_time)
{
	AN_Animator *anim = AN_SystemGetAnimator(handle);

	if (!anim)
		return;

	AN_AnimatorPlay(anim, clip, loop, global_start_time);
}

static b32 AN_IsFinished(AN_Handle handle)
{
	AN_Animator *anim = AN_SystemGetAnimator(handle);
	return anim ? AN_AnimatorIsFinished(anim) : false;
}

static AN_Palette AN_GetPalette(AN_Handle handle, i32 skin_index)
{
	AN_Animator *anim = AN_SystemGetAnimator(handle);

	if (!anim)
	{
		DebugLogW(an_system->log_channel, "Handle is invalid, returning empty palette.");

		AN_Palette empty = {0};
		return empty;
	}

	return AN_AnimatorPalette(anim, skin_index);
}

static AN_Handle AN_SystemCreateInstance(A_Handle model_handle)
{
	AN_Handle result = AN_HandleNull();

	u32 index = 0;

	if (an_system->free_index_count > 0)
	{
		an_system->free_index_count--;
		index = an_system->free_indices[an_system->free_index_count];
	}
	else
	{
		if (an_system->instance_count >= ArraySize(an_system->instances))
		{
			DebugLogE(an_system->log_channel,
					  "Instance pool exhausted (max %u).",
					  ArraySize(an_system->instances));

			return result;
		}

		index = an_system->instance_count;

		an_system->instance_count++;
	}

	AN_Instance *inst = &an_system->instances[index];

	ArenaReset(&inst->arena);

	AN_AnimatorSelect(&inst->animator, &inst->arena, model_handle);

	inst->alive = true;

	result.index = index;
	result.generation = inst->generation;

	return result;
}

static void AN_SystemKillInstance(AN_Handle h)
{
	AN_Instance *inst = AN_SystemResolve(h);

	if (!inst)
		return;

	inst->alive = false;
	inst->generation++;

	an_system->free_indices[an_system->free_index_count] = h.index;
	an_system->free_index_count++;
}
