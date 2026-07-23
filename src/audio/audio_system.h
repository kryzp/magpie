#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

typedef enum AU_Bus
{
	AU_Bus_Music,
	AU_Bus_Sfx,
	AU_Bus_COUNT
}
AU_Bus;

typedef struct AU_Handle AU_Handle;
struct AU_Handle
{
	u32 value;
};

// null handle (value = 0) is reserved
// as the invalid handle!!
internal inline AU_Handle AU_HandleNull(void)
{
	return (AU_Handle) {0};
}

internal inline b32 AU_HandleIsValid(AU_Handle h)
{
	return h.value != 0;
}

internal inline b32 AU_HandleMatch(AU_Handle a, AU_Handle b)
{
	return a.value == b.value;
}

typedef struct AU_Emitter AU_Emitter;
struct AU_Emitter
{
	AU_Emitter *next;
	AU_Emitter *prev;
	
	AU_Handle handle;
	AU_SourceHandle source;
	AU_Bus bus;
	f32 base_volume;
};

typedef struct AU_PlayConfig AU_PlayConfig;
struct AU_PlayConfig
{
	AU_BufferHandle clip;
	AU_Bus bus;
	f32 volume;
	f32 pitch;
	b32 spatial;
	v3 position;
};

typedef struct AU_System AU_System;
struct AU_System
{
	Arena *arena;
	LOG_Channel log_channel;

	AU_Listener listener;

	AU_Emitter emitter_sentinel;
	AU_Emitter free_emitter_sentinel;

	AU_Handle curr_emitter_handle;
	
	f32 master_volume;
	f32 bus_volumes[AU_Bus_COUNT];
};

internal void AU_InitAndSelect(AU_System *system, Arena *arena, LOG_Channel log_channel);
internal void AU_Shutdown(void);
internal void AU_SelectContext(AU_System *system);

internal void AU_Tick(f32 dt, AU_Listener listener);

internal AU_Emitter *AU_AllocEmitter(void);
internal void AU_ReleaseEmitter(AU_Emitter *emitter);
internal AU_Emitter *AU_GetEmitter(AU_Handle handle);

internal AU_Handle AU_Play(const AU_PlayConfig *config);
internal void AU_Stop(AU_Handle handle);
internal void AU_StopAll(void);
internal void AU_Resume(AU_Handle handle);
internal void AU_Pause(AU_Handle handle);

internal void AU_SetPositionOf(AU_Handle handle, v3 position);

internal void AU_SetMasterVolume(f32 volume);
internal void AU_SetBusVolume(AU_Bus bus, f32 volume);

internal f32 AU_GetOutputVolumeOnBus(AU_Bus bus, f32 base_volume);
internal void AU_UpdateEmitterVolumes(AU_Bus bus);

#endif // AUDIO_SYSTEM_H
