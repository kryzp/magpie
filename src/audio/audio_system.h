#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

typedef enum AUD_Bus
{
	AUD_Bus_Music,
	AUD_Bus_Sfx,
	AUD_Bus_COUNT
}
AUD_Bus;

typedef struct AUD_Handle AUD_Handle;
struct AUD_Handle
{
	u32 value;
};

internal inline b32
AUD_HandleMatch(AUD_Handle a, AUD_Handle b)
{
	return a.value == b.value;
}

typedef struct AUD_Voice AUD_Voice;
struct AUD_Voice
{
	AUD_Voice *next;
	AUD_Voice *prev;
	
	AUD_Handle handle;
	AUD_SourceHandle source;
	AUD_Bus bus;
	f32 base_volume;
};

typedef struct AUD_System AUD_System;
struct AUD_System
{
	Arena *arena;
	AUD_BackendAPI *api;

	AUD_Listener listener;
	
	AUD_Voice *voices;
	AUD_Voice *free_voices;

	AUD_Handle curr_voice_handle;
	
	f32 master_volume;
	f32 bus_volumes[AUD_Bus_COUNT];
};

internal void AUD_Init(AUD_System *system, Arena *arena, AUD_BackendAPI *api);
internal void AUD_Shutdown(AUD_System *system);

internal void AUD_Tick(AUD_System *system, f32 dt, AUD_Listener listener);

internal AUD_Voice *AUD_AllocVoice(AUD_System *system);
internal AUD_Voice *AUD_GetVoice(const AUD_System *system, AUD_Handle handle);

internal AUD_Handle AUD_PlaySound(AUD_System *system,
								  AUD_BufferHandle clip,
								  AUD_Bus bus,
								  f32 volume, f32 pitch);

internal AUD_Handle AUD_PlaySound3D(AUD_System *system,
									AUD_BufferHandle clip,
									AUD_Bus bus,
									v3 position,
									f32 volume, f32 pitch);

internal void AUD_Stop(AUD_System *system, AUD_Handle handle);
internal void AUD_StopAll(AUD_System *system);

internal void AUD_SetSoundPosition(const AUD_System *system, AUD_Handle handle, v3 position);

internal void AUD_SetMasterVolume(AUD_System *system, f32 volume);
internal void AUD_SetBusVolume(AUD_System *system, AUD_Bus bus, f32 volume);

internal f32 AUD_GetOutputVolumeOnBus(const AUD_System *system, AUD_Bus bus, f32 base_volume);
internal void AUD_UpdateVoiceVolumes(const AUD_System *system, AUD_Bus bus);

#endif // AUDIO_SYSTEM_H
