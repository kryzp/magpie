#ifndef AUDIO_BACKEND_H
#define AUDIO_BACKEND_H

typedef struct AU_BufferHandle { u32 value; } AU_BufferHandle;
typedef struct AU_SourceHandle { u32 value; } AU_SourceHandle;

internal inline b32 AU_BufferHandleMatch(AU_BufferHandle a, AU_BufferHandle b)
{
	return a.value == b.value;
}

internal inline b32 AU_SourceHandleMatch(AU_SourceHandle a, AU_SourceHandle b)
{
	return a.value == b.value;
}

typedef enum AU_Format
{
	AU_Format_U8,
	AU_Format_S16,
	AU_Format_S24,
	AU_Format_S32,
	AU_Format_F32,
	AU_Format_COUNT
}
AU_Format;

typedef enum AU_AttenuationModel
{
	AU_AttenuationModel_Inverse,
	AU_AttenuationModel_Exponential,
	AU_AttenuationModel_Linear,
	AU_AttenuationModel_COUNT
}
AU_AttenuationModel;

typedef struct AU_Backend AU_Backend;

internal AU_Backend *AU_BackendAllocAndSelect(Arena *arena, LOG_Channel log_channel);
internal void AU_BackendShutdown(void);
internal void AU_BackendSelectContext(AU_Backend *backend);

internal void AU_BackendTick(f32 dt, AU_Listener listener);

internal void AU_BackendPlay(AU_SourceHandle handle);
internal void AU_BackendStop(AU_SourceHandle handle);
internal void AU_BackendResume(AU_SourceHandle handle);
internal void AU_BackendPause(AU_SourceHandle handle);
internal void AU_BackendReset(AU_SourceHandle handle);

internal b32 AU_BackendIsPlaying(AU_SourceHandle handle);
internal b32 AU_BackendIsLooping(AU_SourceHandle handle);

internal AU_BufferHandle AU_BackendCreateBuffer(const void *data, u64 bytes, u32 channels, u16 sample_rate, AU_Format format);
internal void AU_BackendDestroyBuffer(AU_BufferHandle handle);

internal AU_SourceHandle AU_BackendCreateSourceFromBuffer(AU_BufferHandle handle);
// TODO: CreateSourceFromStream
internal void AU_BackendDestroySource(AU_SourceHandle handle);

internal void AU_BackendSetSourceVolume(AU_SourceHandle handle, f32 volume);
internal void AU_BackendSetSourcePitch(AU_SourceHandle handle, f32 pitch);
internal void AU_BackendSetSourceLooping(AU_SourceHandle handle, b32 loop);
internal void AU_BackendSetSourcePosition(AU_SourceHandle handle, v3 position);
internal void AU_BackendSetSourceDopplerFactor(AU_SourceHandle handle, f32 factor);
internal void AU_BackendSetSourceAttenuationModel(AU_SourceHandle handle, AU_AttenuationModel model);
internal void AU_BackendSetSourceAttenuationRange(AU_SourceHandle handle, f32 dist_min, f32 dist_max);

#endif // AUDIO_BACKEND_H
