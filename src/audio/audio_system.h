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

// null handle (value = 0) is reserved
// as the invalid handle!!
internal inline AUD_Handle
AUD_HandleNull(void)
{
	return (AUD_Handle) {0};
}

internal inline b32
AUD_HandleIsValid(AUD_Handle h)
{
	return h.value != 0;
}

internal inline b32
AUD_HandleMatch(AUD_Handle a, AUD_Handle b)
{
	return a.value == b.value;
}

typedef struct AUD_Emitter AUD_Emitter;
struct AUD_Emitter
{
	AUD_Emitter *next;
	AUD_Emitter *prev;
	
	AUD_Handle handle;
	AUD_SourceHandle source;
	AUD_Bus bus;
	f32 base_volume;
};

typedef struct AUD_PlayConfig AUD_PlayConfig;
struct AUD_PlayConfig
{
	AUD_BufferHandle clip;
	AUD_Bus bus;
	f32 volume;
	f32 pitch;
	b32 spatial;
	v3 position;
};

typedef struct AUD_System AUD_System;
struct AUD_System
{
	Arena *arena;
	AUD_BackendAPI *api;

	LOG_Channel log_channel;

	AUD_Listener listener;

	AUD_Emitter emitter_sentinel;
	AUD_Emitter free_emitter_sentinel;

	AUD_Handle curr_emitter_handle;
	
	f32 master_volume;
	f32 bus_volumes[AUD_Bus_COUNT];
};

internal void AUD_Init     (AUD_System *system, Arena *arena, AUD_BackendAPI *api);
internal void AUD_Shutdown (AUD_System *system);

internal void AUD_Tick(AUD_System *system, f32 dt, AUD_Listener listener);

internal AUD_Emitter *AUD_AllocEmitter   (AUD_System *system);
internal void         AUD_ReleaseEmitter (AUD_System *system, AUD_Emitter *emitter);
internal AUD_Emitter *AUD_GetEmitter     (const AUD_System *system, AUD_Handle handle);

internal AUD_Handle AUD_Play    (AUD_System *system, const AUD_PlayConfig *config);
internal void       AUD_Stop    (AUD_System *system, AUD_Handle handle);
internal void       AUD_StopAll (AUD_System *system);
internal void       AUD_Resume  (AUD_System *system, AUD_Handle handle);
internal void       AUD_Pause   (AUD_System *system, AUD_Handle handle);

internal void AUD_SetPositionOf(const AUD_System *system, AUD_Handle handle, v3 position);

internal void AUD_SetMasterVolume (AUD_System *system, f32 volume);
internal void AUD_SetBusVolume    (AUD_System *system, AUD_Bus bus, f32 volume);

internal f32  AUD_GetOutputVolumeOnBus (const AUD_System *system, AUD_Bus bus, f32 base_volume);
internal void AUD_UpdateEmitterVolumes (const AUD_System *system, AUD_Bus bus);

#endif // AUDIO_SYSTEM_H
