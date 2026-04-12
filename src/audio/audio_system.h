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
	u32 index;
	u32 generation;
};

typedef struct AUD_FreeIndex AUD_FreeIndex;
struct AUD_FreeIndex
{
	AUD_FreeIndex *next;
	u32 value;
};

typedef struct AUD_Voice AUD_Voice;
struct AUD_Voice
{
	AUD_Voice *pool_next;
	AUD_Handle handle;
	AUD_SourceHandle source;
	AUD_Bus bus;
	f32 base_volume;
	b32 active;
};

typedef struct AUD_System AUD_System;
struct AUD_System
{
	Arena *arena;
	AUD_BackendAPI *api;

	f32 master_volume;
	f32 bus_volumes[AUD_Bus_COUNT];
	
	AUD_Listener listener;

	AUD_Voice *voice_pool;
	AUD_FreeIndex *free_indices;
};

internal void AUD_Init(AUD_System *system, Arena *arena, AUD_BackendAPI *api);
internal void AUD_Shutdown(AUD_System *system);

internal void AUD_Tick(AUD_System *system, f32 dt, AUD_Listener listener);

internal b32 AUD_IsValidHandle(const AUD_System *system, AUD_Handle handle);

internal AUD_Handle AUD_PlaySound(AUD_System *system,
								  AUD_BufferHandle clip,
								  AUD_Bus bus,
								  f32 volume, f32 pitch);

internal AUD_Handle AUD_PlaySound3D(AUD_System *system,
									AUD_BufferHandle clip,
									AUD_Bus bus,
									v3 position,
									f32 volume, f32 pitch);

internal void AUD_Stop(const AUD_System *system, AUD_Handle handle);
internal void AUD_StopAll(const AUD_System *system);

internal void AUD_SetSoundPosition(const AUD_System *system, AUD_Handle handle, v3 position);

internal void AUD_SetMasterVolume(const AUD_System *system, f32 volume);
internal void AUD_SetBusVolume(const AUD_System *system, AUD_Bus bus, f32 volume);

internal f32 AUD_GetOutputVolumeOnBus(const AUD_System *system, AUD_Bus bus, f32 base_volume);
internal void AUD_UpdateVoiceVolumes(const AUD_SYSTEM *system, AUD_Bus bus);

#endif // AUDIO_SYSTEM_H
