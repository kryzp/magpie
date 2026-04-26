internal void
AUD_Init(AUD_System *system, Arena *arena, LOG_Channel log_channel, AUD_BackendAPI *api)
{
	system->arena = arena;
	system->api = api;

	system->log_channel = log_channel;

	system->master_volume = 1.f;

	for (u32 b = 0; b < AUD_Bus_COUNT; b++)
		system->bus_volumes[b] = 1.f;

	system->curr_emitter_handle.value = 1;

	system->emitter_sentinel.next = &system->emitter_sentinel;
	system->emitter_sentinel.prev = &system->emitter_sentinel;
	system->free_emitter_sentinel.next = &system->free_emitter_sentinel;
	system->free_emitter_sentinel.prev = &system->free_emitter_sentinel;

	DebugLogI(system->log_channel, "Initialized.");
}

internal void
AUD_Shutdown(AUD_System *system)
{
	AUD_StopAll(system);
	
	DebugLogI(system->log_channel, "Shut Down.");
}

internal void
AUD_Tick(AUD_System *system, f32 dt, AUD_Listener listener)
{
	system->api->Tick(dt, listener);
}

internal AUD_Emitter *
AUD_AllocEmitter(AUD_System *system)
{
	AUD_Emitter *emitter;

	if (system->free_emitter_sentinel.next != &system->free_emitter_sentinel)
	{
		emitter = system->free_emitter_sentinel.next;
		emitter->prev->next = emitter->next;
		emitter->next->prev = emitter->prev;

		MemZeroStruct(emitter);
	}
	else
	{
		emitter = ArenaPushArray(system->arena, AUD_Emitter, 1);
	}

	emitter->handle = system->curr_emitter_handle;
	system->curr_emitter_handle.value++;

	emitter->next = system->emitter_sentinel.next;
	emitter->prev = &system->emitter_sentinel;
	emitter->next->prev = emitter;
	emitter->prev->next = emitter;

	return emitter;
}

internal void
AUD_ReleaseEmitter(AUD_System *system, AUD_Emitter *emitter)
{
	emitter->prev->next = emitter->next;
	emitter->next->prev = emitter->prev;

	emitter->next = system->free_emitter_sentinel.next;
	emitter->prev = &system->free_emitter_sentinel;
	emitter->next->prev = emitter;
	emitter->prev->next = emitter;
}

internal AUD_Emitter *
AUD_GetEmitter(const AUD_System *system, AUD_Handle handle)
{
	const AUD_Emitter *sentinel = &system->emitter_sentinel;

	for (AUD_Emitter *emitter = sentinel->next; emitter != sentinel; emitter = emitter->next)
	{
		if (AUD_HandleMatch(handle, emitter->handle))
			return emitter;
	}

	return NULL;
}

internal AUD_Handle
AUD_Play(AUD_System *system, const AUD_PlayConfig *config)
{
	AUD_SourceHandle source = system->api->CreateSourceFromBuffer(config->clip);
	system->api->SetSourceVolume(source, AUD_GetOutputVolumeOnBus(system, config->bus, config->volume));
	system->api->SetSourcePitch(source, config->pitch);

	if (config->spatial)
		system->api->SetSourcePosition(source, config->position);

	system->api->Play(source);

	AUD_Emitter *emitter = AUD_AllocEmitter(system);
	emitter->source = source;
	emitter->bus = config->bus;
	emitter->base_volume = config->volume;
	
	return emitter->handle;
}

internal void
AUD_Stop(AUD_System *system, AUD_Handle handle)
{
	AUD_Emitter *emitter = AUD_GetEmitter(system, handle);
	AssertTrue(emitter);

	system->api->Stop(emitter->source);
	system->api->DestroySource(emitter->source);

	AUD_ReleaseEmitter(system, emitter);
}

internal void
AUD_StopAll(AUD_System *system)
{
	AUD_Emitter *sentinel = &system->emitter_sentinel;
	AUD_Emitter *emitter    = sentinel->next;

	while (emitter != sentinel)
	{
		AUD_Emitter *next = emitter->next;

		system->api->Stop(emitter->source);
		system->api->DestroySource(emitter->source);

		AUD_ReleaseEmitter(system, emitter);

		emitter = next;
	}
}

internal void
AUD_Resume(AUD_System *system, AUD_Handle handle)
{
	AUD_Emitter *emitter = AUD_GetEmitter(system, handle);
	AssertTrue(emitter);

	system->api->Resume(emitter->source);
}

internal void
AUD_Pause(AUD_System *system, AUD_Handle handle)
{
	AUD_Emitter *emitter = AUD_GetEmitter(system, handle);
	AssertTrue(emitter);

	system->api->Pause(emitter->source);
}

internal void
AUD_SetPositionOf(const AUD_System *system, AUD_Handle handle, v3 position)
{
	AUD_Emitter *emitter = AUD_GetEmitter(system, handle);
	AssertTrue(emitter);

	system->api->SetSourcePosition(emitter->source, position);
}

internal void
AUD_SetMasterVolume(AUD_System *system, f32 volume)
{
	system->master_volume = volume;

	for (u32 b = 0; b < AUD_Bus_COUNT; b++)
		AUD_UpdateEmitterVolumes(system, b);
}

internal void
AUD_SetBusVolume(AUD_System *system, AUD_Bus bus, f32 volume)
{
	system->bus_volumes[bus] = volume;
	AUD_UpdateEmitterVolumes(system, bus);
}

internal f32
AUD_GetOutputVolumeOnBus(const AUD_System *system, AUD_Bus bus, f32 base_volume)
{
	return base_volume * system->bus_volumes[bus] * system->master_volume;
}

internal void
AUD_UpdateEmitterVolumes(const AUD_System *system, AUD_Bus bus)
{
	const AUD_Emitter *sentinel = &system->emitter_sentinel;

	for (AUD_Emitter *emitter = sentinel->next; emitter != sentinel; emitter = emitter->next)
	{
		if (emitter->bus == bus)
			system->api->SetSourceVolume(emitter->source, AUD_GetOutputVolumeOnBus(system, bus, emitter->base_volume));
	}
}
