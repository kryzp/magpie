
internal void
AUD_Init(AUD_System *system, Arena *arena, AUD_BackendAPI *api)
{
	system->arena = arena;
	system->api = api;

	system->master_volume = 1.f;

	for (u32 b = 0; b < AUD_Bus_COUNT; b++)
		system->bus_volumes[b] = 1.f;
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
	AUD_Voice *voice = system->free_voices;

	if (voice)
	{
		system->free_voices = system->free_voices->next;
		MemZeroStruct(voice);
	}
	else
	{
		voice = ArenaPushArray(system->arena, AUD_Voice, 1);
	}

	voice->handle = system->curr_voice_handle;
	system->curr_voice_handle.value++;

	voice->next = system->voices;
	system->voices = voice;

	return voice;
}

internal AUD_Voice *
AUD_GetVoice(const AUD_System *system, AUD_Handle handle)
{
	for (AUD_Voice *voice = system->voices; voice; voice = voice->next)
	{
		if (AUD_HandleMatch(handle, voice->handle))
			return voice;
	}

	return NULL;
}

internal AUD_Handle
AUD_PlaySound(AUD_System *system,
			  AUD_BufferHandle clip,
			  AUD_Bus bus,
			  f32 volume, f32 pitch)
{
	AUD_SourceHandle source = system->api->CreateSourceFromBuffer(clip);
	system->api->SetSourceVolume(source, AUD_GetOutputVolumeOnBus(system, bus, volume));
	system->api->SetSourcePitch(source, pitch);

	system->api->Play(source);

	AUD_Voice *voice = AUD_AllocVoice(system);
	voice->source = source;
	voice->bus = bus;
	voice->base_volume = volume;
	
	return voice->handle;
}

internal AUD_Handle
AUD_PlaySound3D(AUD_System *system,
				AUD_BufferHandle clip,
				AUD_Bus bus,
				v3 position,
				f32 volume, f32 pitch)
{
	AUD_SourceHandle source = system->api->CreateSourceFromBuffer(clip);
	system->api->SetSourcePosition(source, position);
	system->api->SetSourceVolume(source, AUD_GetOutputVolumeOnBus(system, bus, volume));
	system->api->SetSourcePitch(source, pitch);

	system->api->Play(source);

	AUD_Voice *voice = AUD_AllocVoice(system);
	voice->source = source;
	voice->bus = bus;
	voice->base_volume = volume;
	
	return voice->handle;
}

internal void
AUD_Stop(AUD_System *system, AUD_Handle handle)
{
	AUD_Voice *voice = AUD_GetVoice(system, handle);
	AssertTrue(voice);
	
	system->api->Stop(voice->source);
	system->api->DestroySource(voice->source);
	
	voice->next = system->free_voices;
	system->free_voices->prev = voice;
	system->free_voices = voice;
}

internal void
AUD_StopAll(AUD_System *system)
{
	for (AUD_Voice *voice = system->voices; voice; voice = voice->next)
	{
		system->api->Stop(voice->source);
		system->api->DestroySource(voice->source);
	
		voice->next = system->free_voices;
		system->free_voices->prev = voice;
		system->free_voices = voice;
	}
}

internal void
AUD_SetSoundPosition(const AUD_System *system, AUD_Handle handle, v3 position)
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
	for (AUD_Voice *voice = system->voices; voice; voice = voice->next)
	{
		if (voice->bus == bus)
			system->api->SetSourceVolume(voice->source, AUD_GetOutputVolumeOnBus(system, bus, voice->base_volume));
	}
}
