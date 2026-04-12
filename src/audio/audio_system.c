
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
	system->backend->Tick(dt, listener);
}

internal b32
AUD_IsValidHandle(const AUD_System *system, AUD_Handle handle)
{
}

internal AUD_Handle
AUD_PlaySound(AUD_System *system,
			  AUD_BufferHandle clip,
			  AUD_Bus bus,
			  f32 volume, f32 pitch)
{
}

internal AUD_Handle
AUD_PlaySound3D(AUD_System *system,
				AUD_BufferHandle clip,
				AUD_Bus bus,
				v3 position,
				f32 volume, f32 pitch)
{
}

internal void
AUD_Stop(const AUD_System *system, AUD_Handle handle)
{
}

internal void
AUD_StopAll(const AUD_System *system)
{
}

internal void
AUD_SetSoundPosition(const AUD_System *system, AUD_Handle handle, v3 position)
{
}

internal void
AUD_SetMasterVolume(const AUD_System *system, f32 volume)
{
	system->master_volume = volume;

	for (u32 b = 0; b < AUD_Bus_COUNT; b++)
		AUD_UpdateVoiceVolumes(b);
}

internal void
AUD_SetBusVolume(const AUD_System *system, AUD_Bus bus, f32 volume)
{
	system->buf_volumes[bus] = volume;
	AUD_UpdateVoiceVolumes(system, bus);
}

internal f32
AUD_GetOutputVolumeOnBus(const AUD_System *system, AUD_Bus bus, f32 base_volume)
{
	return system->base_volume * system->bus_volumes[bus] * system->master_volume;
}

internal void
AUD_UpdateVoiceVolumes(const AUD_SYSTEM *system, AUD_Bus bus)
{
}
