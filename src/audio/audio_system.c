
static void AU_Init(AU_System *system, Arena *arena, LOG_Channel log_channel, AU_Backend *backend)
{
	system->arena = arena;
	system->backend = backend;

	system->log_channel = log_channel;

	system->master_volume = 1.f;

	for (u32 b = 0; b < AU_Bus_COUNT; b++)
		system->bus_volumes[b] = 1.f;

	system->curr_emitter_handle.value = 1;

	system->emitter_sentinel.next = &system->emitter_sentinel;
	system->emitter_sentinel.prev = &system->emitter_sentinel;
	system->free_emitter_sentinel.next = &system->free_emitter_sentinel;
	system->free_emitter_sentinel.prev = &system->free_emitter_sentinel;

	DebugLogI(system->log_channel, "Initialized.");
}

static void AU_Shutdown(AU_System *system)
{
	DebugLogI(system->log_channel, "Shutting Down...");
	
	AU_StopAll(system);
}

static void AU_Tick(AU_System *system, f32 dt, AU_Listener listener)
{
}

static AU_Emitter *AU_AllocEmitter(AU_System *system)
{
	AU_Emitter *emitter;

	if (system->free_emitter_sentinel.next != &system->free_emitter_sentinel)
	{
		emitter = system->free_emitter_sentinel.next;
		emitter->prev->next = emitter->next;
		emitter->next->prev = emitter->prev;

		MemZeroStruct(emitter);
	}
	else
	{
		emitter = ArenaPushArray(system->arena, AU_Emitter, 1);
	}

	emitter->handle = system->curr_emitter_handle;
	system->curr_emitter_handle.value++;

	emitter->next = system->emitter_sentinel.next;
	emitter->prev = &system->emitter_sentinel;
	emitter->next->prev = emitter;
	emitter->prev->next = emitter;

	return emitter;
}

static void AU_ReleaseEmitter(AU_System *system, AU_Emitter *emitter)
{
	emitter->prev->next = emitter->next;
	emitter->next->prev = emitter->prev;

	emitter->next = system->free_emitter_sentinel.next;
	emitter->prev = &system->free_emitter_sentinel;
	emitter->next->prev = emitter;
	emitter->prev->next = emitter;
}

static AU_Emitter *AU_GetEmitter(const AU_System *system, AU_Handle handle)
{
	const AU_Emitter *sentinel = &system->emitter_sentinel;

	for (AU_Emitter *emitter = sentinel->next; emitter != sentinel; emitter = emitter->next)
	{
		if (AU_HandleMatch(handle, emitter->handle))
			return emitter;
	}

	return NULL;
}

static AU_Handle AU_Play(AU_System *system, const AU_PlayConfig *config)
{
	AU_SourceHandle source = AU_BackendCreateSourceFromBuffer(system->backend, config->clip);
	AU_BackendSetSourceVolume(system->backend, source, AU_GetOutputVolumeOnBus(system, config->bus, config->volume));
	AU_BackendSetSourcePitch(system->backend, source, config->pitch);

	if (config->spatial)
		AU_BackendSetSourcePosition(system->backend, source, config->position);

	AU_BackendPlay(system->backend, source);

	AU_Emitter *emitter = AU_AllocEmitter(system);
	emitter->source = source;
	emitter->bus = config->bus;
	emitter->base_volume = config->volume;
	
	return emitter->handle;
}

static void AU_Stop(AU_System *system, AU_Handle handle)
{
	AU_Emitter *emitter = AU_GetEmitter(system, handle);
	
	DebugLogAssert(system->log_channel, emitter, "Invalid handle.");

	AU_BackendStop(system->backend, emitter->source);
	AU_BackendDestroySource(system->backend, emitter->source);

	AU_ReleaseEmitter(system, emitter);
}

static void AU_StopAll(AU_System *system)
{
	AU_Emitter *sentinel = &system->emitter_sentinel;
	AU_Emitter *emitter = sentinel->next;

	while (emitter != sentinel)
	{
		AU_Emitter *next = emitter->next;

		AU_BackendStop(system->backend, emitter->source);
		AU_BackendDestroySource(system->backend, emitter->source);

		AU_ReleaseEmitter(system, emitter);

		emitter = next;
	}
}

static void AU_Resume(AU_System *system, AU_Handle handle)
{
	AU_Emitter *emitter = AU_GetEmitter(system, handle);
	
	DebugLogAssert(system->log_channel, emitter, "Invalid handle.");

	AU_BackendResume(system->backend, emitter->source);
}

static void AU_Pause(AU_System *system, AU_Handle handle)
{
	AU_Emitter *emitter = AU_GetEmitter(system, handle);
	
	DebugLogAssert(system->log_channel, emitter, "Invalid handle.");

	AU_BackendPause(system->backend, emitter->source);
}

static void AU_SetPositionOf(const AU_System *system, AU_Handle handle, v3 position)
{
	AU_Emitter *emitter = AU_GetEmitter(system, handle);
	
	DebugLogAssert(system->log_channel, emitter, "Invalid handle.");

	AU_BackendSetSourcePosition(system->backend, emitter->source, position);
}

static void AU_SetMasterVolume(AU_System *system, f32 volume)
{
	system->master_volume = volume;

	for (u32 b = 0; b < AU_Bus_COUNT; b++)
		AU_UpdateEmitterVolumes(system, b);
}

static void AU_SetBusVolume(AU_System *system, AU_Bus bus, f32 volume)
{
	system->bus_volumes[bus] = volume;
	AU_UpdateEmitterVolumes(system, bus);
}

static f32 AU_GetOutputVolumeOnBus(const AU_System *system, AU_Bus bus, f32 base_volume)
{
	return base_volume * system->bus_volumes[bus] * system->master_volume;
}

static void AU_UpdateEmitterVolumes(const AU_System *system, AU_Bus bus)
{
	const AU_Emitter *sentinel = &system->emitter_sentinel;

	for (AU_Emitter *emitter = sentinel->next; emitter != sentinel; emitter = emitter->next)
	{
		if (emitter->bus == bus)
			AU_BackendSetSourceVolume(system->backend, emitter->source, AU_GetOutputVolumeOnBus(system, bus, emitter->base_volume));
	}
}
