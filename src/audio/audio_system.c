internal void
AUD_Init(AUD_System *system, Arena *arena, AUD_BackendAPI *api)
{
	system->arena = arena;
	system->api = api;

	system->master_volume = 1.f;

	for (u32 b = 0; b < AUD_Bus_COUNT; b++)
		system->bus_volumes[b] = 1.f;

	system->curr_voice_handle.value = 1;

	system->voice_sentinel.next = &system->voice_sentinel;
	system->voice_sentinel.prev = &system->voice_sentinel;
	system->free_voice_sentinel.next = &system->free_voice_sentinel;
	system->free_voice_sentinel.prev = &system->free_voice_sentinel;
}

internal void
AUD_Shutdown(AUD_System *system)
{
	AUD_StopAll(system);
}

internal void
AUD_Tick(AUD_System *system, f32 dt, AUD_Listener listener)
{
	system->api->Tick(dt, listener);
}

internal AUD_Voice *
AUD_AllocVoice(AUD_System *system)
{
	AUD_Voice *voice;

	if (system->free_voice_sentinel.next != &system->free_voice_sentinel)
	{
		voice = system->free_voice_sentinel.next;
		voice->prev->next = voice->next;
		voice->next->prev = voice->prev;

		MemZeroStruct(voice);
	}
	else
	{
		voice = ArenaPushArray(system->arena, AUD_Voice, 1);
	}

	voice->handle = system->curr_voice_handle;
	system->curr_voice_handle.value++;

	voice->next = system->voice_sentinel.next;
	voice->prev = &system->voice_sentinel;
	voice->next->prev = voice;
	voice->prev->next = voice;

	return voice;
}

internal void
AUD_ReleaseVoice(AUD_System *system, AUD_Voice *voice)
{
	voice->prev->next = voice->next;
	voice->next->prev = voice->prev;

	voice->next = system->free_voice_sentinel.next;
	voice->prev = &system->free_voice_sentinel;
	voice->next->prev = voice;
	voice->prev->next = voice;
}

internal AUD_Voice *
AUD_GetVoice(const AUD_System *system, AUD_Handle handle)
{
	const AUD_Voice *sentinel = &system->voice_sentinel;

	for (AUD_Voice *voice = sentinel->next; voice != sentinel; voice = voice->next)
	{
		if (AUD_HandleMatch(handle, voice->handle))
			return voice;
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

	AUD_Voice *voice = AUD_AllocVoice(system);
	voice->source = source;
	voice->bus = config->bus;
	voice->base_volume = config->volume;
	
	return voice->handle;
}

internal void
AUD_Stop(AUD_System *system, AUD_Handle handle)
{
	AUD_Voice *voice = AUD_GetVoice(system, handle);
	AssertTrue(voice);

	system->api->Stop(voice->source);
	system->api->DestroySource(voice->source);

	AUD_ReleaseVoice(system, voice);
}

internal void
AUD_StopAll(AUD_System *system)
{
	AUD_Voice *sentinel = &system->voice_sentinel;
	AUD_Voice *voice    = sentinel->next;

	while (voice != sentinel)
	{
		AUD_Voice *next = voice->next;

		system->api->Stop(voice->source);
		system->api->DestroySource(voice->source);

		AUD_ReleaseVoice(system, voice);

		voice = next;
	}
}

internal void
AUD_Resume(AUD_System *system, AUD_Handle handle)
{
	AUD_Voice *voice = AUD_GetVoice(system, handle);
	AssertTrue(voice);

	system->api->Resume(voice->source);
}

internal void
AUD_Pause(AUD_System *system, AUD_Handle handle)
{
	AUD_Voice *voice = AUD_GetVoice(system, handle);
	AssertTrue(voice);

	system->api->Pause(voice->source);
}

internal void
AUD_SetPositionOf(const AUD_System *system, AUD_Handle handle, v3 position)
{
	AUD_Voice *voice = AUD_GetVoice(system, handle);
	AssertTrue(voice);

	system->api->SetSourcePosition(voice->source, position);
}

internal void
AUD_SetMasterVolume(AUD_System *system, f32 volume)
{
	system->master_volume = volume;

	for (u32 b = 0; b < AUD_Bus_COUNT; b++)
		AUD_UpdateVoiceVolumes(system, b);
}

internal void
AUD_SetBusVolume(AUD_System *system, AUD_Bus bus, f32 volume)
{
	system->bus_volumes[bus] = volume;
	AUD_UpdateVoiceVolumes(system, bus);
}

internal f32
AUD_GetOutputVolumeOnBus(const AUD_System *system, AUD_Bus bus, f32 base_volume)
{
	return base_volume * system->bus_volumes[bus] * system->master_volume;
}

internal void
AUD_UpdateVoiceVolumes(const AUD_System *system, AUD_Bus bus)
{
	const AUD_Voice *sentinel = &system->voice_sentinel;

	for (AUD_Voice *voice = sentinel->next; voice != sentinel; voice = voice->next)
	{
		if (voice->bus == bus)
			system->api->SetSourceVolume(voice->source, AUD_GetOutputVolumeOnBus(system, bus, voice->base_volume));
	}
}
