
static AU_System *au_system = NULL;

static void AU_InitAndSelect(AU_System *system, Arena *arena, LOG_Channel log_channel)
{
	system->arena = arena;
	system->log_channel = log_channel;

	system->master_volume = 1.f;

	for (u32 b = 0; b < AU_Bus_COUNT; b++)
		system->bus_volumes[b] = 1.f;

	system->curr_emitter_handle.value = 1;

	system->emitter_sentinel.next = &system->emitter_sentinel;
	system->emitter_sentinel.prev = &system->emitter_sentinel;
	system->free_emitter_sentinel.next = &system->free_emitter_sentinel;
	system->free_emitter_sentinel.prev = &system->free_emitter_sentinel;

	AU_SelectContext(system);

	DebugLogI(system->log_channel, "Initialized.");
}

static void AU_Shutdown(void)
{
	AU_StopAll();

	DebugLogI(au_system->log_channel, "Destroyed.");

	au_system = NULL;
}

static void AU_SelectContext(AU_System *system)
{
	au_system = system;
}

static void AU_Tick(f32 dt, AU_Listener listener)
{
}

static AU_Emitter *AU_AllocEmitter(void)
{
	AU_Emitter *emitter;

	if (au_system->free_emitter_sentinel.next != &au_system->free_emitter_sentinel)
	{
		emitter = au_system->free_emitter_sentinel.next;
		emitter->prev->next = emitter->next;
		emitter->next->prev = emitter->prev;

		MemZeroStruct(emitter);
	}
	else
	{
		emitter = ArenaPushArray(au_system->arena, AU_Emitter, 1);
	}

	emitter->handle = au_system->curr_emitter_handle;
	au_system->curr_emitter_handle.value++;

	emitter->next = au_system->emitter_sentinel.next;
	emitter->prev = &au_system->emitter_sentinel;
	emitter->next->prev = emitter;
	emitter->prev->next = emitter;

	return emitter;
}

static void AU_ReleaseEmitter(AU_Emitter *emitter)
{
	emitter->prev->next = emitter->next;
	emitter->next->prev = emitter->prev;

	emitter->next = au_system->free_emitter_sentinel.next;
	emitter->prev = &au_system->free_emitter_sentinel;
	emitter->next->prev = emitter;
	emitter->prev->next = emitter;
}

static AU_Emitter *AU_GetEmitter(AU_Handle handle)
{
	const AU_Emitter *sentinel = &au_system->emitter_sentinel;

	for (AU_Emitter *emitter = sentinel->next; emitter != sentinel; emitter = emitter->next)
	{
		if (AU_HandleMatch(handle, emitter->handle))
			return emitter;
	}

	return NULL;
}

static AU_Handle AU_Play(const AU_PlayConfig *config)
{
	AU_SourceHandle source = AU_BackendCreateSourceFromBuffer(config->clip);
	AU_BackendSetSourceVolume(source, AU_GetOutputVolumeOnBus(config->bus, config->volume));
	AU_BackendSetSourcePitch(source, config->pitch);

	if (config->spatial)
		AU_BackendSetSourcePosition(source, config->position);

	AU_BackendPlay(source);

	AU_Emitter *emitter = AU_AllocEmitter();
	emitter->source = source;
	emitter->bus = config->bus;
	emitter->base_volume = config->volume;
	
	return emitter->handle;
}

static void AU_Stop(AU_Handle handle)
{
	AU_Emitter *emitter = AU_GetEmitter(handle);
	
	DebugLogAssert(au_system->log_channel, emitter, "Invalid handle.");

	AU_BackendStop(emitter->source);
	AU_BackendDestroySource(emitter->source);

	AU_ReleaseEmitter(emitter);
}

static void AU_StopAll(void)
{
	AU_Emitter *sentinel = &au_system->emitter_sentinel;
	AU_Emitter *emitter = sentinel->next;

	while (emitter != sentinel)
	{
		AU_Emitter *next = emitter->next;

		AU_BackendStop(emitter->source);
		AU_BackendDestroySource(emitter->source);

		AU_ReleaseEmitter(emitter);

		emitter = next;
	}
}

static void AU_Resume(AU_Handle handle)
{
	AU_Emitter *emitter = AU_GetEmitter(handle);
	
	DebugLogAssert(au_system->log_channel, emitter, "Invalid handle.");

	AU_BackendResume(emitter->source);
}

static void AU_Pause(AU_Handle handle)
{
	AU_Emitter *emitter = AU_GetEmitter(handle);
	
	DebugLogAssert(au_system->log_channel, emitter, "Invalid handle.");

	AU_BackendPause(emitter->source);
}

static void AU_SetPositionOf(AU_Handle handle, v3 position)
{
	AU_Emitter *emitter = AU_GetEmitter(handle);
	
	DebugLogAssert(au_system->log_channel, emitter, "Invalid handle.");

	AU_BackendSetSourcePosition(emitter->source, position);
}

static void AU_SetMasterVolume(f32 volume)
{
	au_system->master_volume = volume;

	for (u32 b = 0; b < AU_Bus_COUNT; b++)
		AU_UpdateEmitterVolumes(b);
}

static void AU_SetBusVolume(AU_Bus bus, f32 volume)
{
	au_system->bus_volumes[bus] = volume;
	AU_UpdateEmitterVolumes(bus);
}

static f32 AU_GetOutputVolumeOnBus(AU_Bus bus, f32 base_volume)
{
	return base_volume * au_system->bus_volumes[bus] * au_system->master_volume;
}

static void AU_UpdateEmitterVolumes(AU_Bus bus)
{
	const AU_Emitter *sentinel = &au_system->emitter_sentinel;

	for (AU_Emitter *emitter = sentinel->next; emitter != sentinel; emitter = emitter->next)
	{
		if (emitter->bus == bus)
			AU_BackendSetSourceVolume(emitter->source, AU_GetOutputVolumeOnBus(bus, emitter->base_volume));
	}
}
