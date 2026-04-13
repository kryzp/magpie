#ifndef AUDIO_BACKEND_H
#define AUDIO_BACKEND_H

typedef struct AUD_BufferHandle { u32 value; } AUD_BufferHandle;
typedef struct AUD_SourceHandle { u32 value; } AUD_SourceHandle;

internal inline b32
AUD_BufferHandleMatch(AUD_BufferHandle a, AUD_BufferHandle b)
{
	return a.value == b.value;
}

internal inline b32
AUD_SourceHandleMatch(AUD_SourceHandle a, AUD_SourceHandle b)
{
	return a.value == b.value;
}

typedef enum AUD_Format
{
	AUD_Format_U8,
	AUD_Format_S16,
	AUD_Format_S24,
	AUD_Format_S32,
	AUD_Format_F32,
	AUD_Format_COUNT
}
AUD_Format;

typedef enum AUD_AttenuationModel
{
	AUD_AttenuationModel_Inverse,
	AUD_AttenuationModel_Exponential,
	AUD_AttenuationModel_Linear,
	AUD_AttenuationModel_COUNT
}
AUD_AttenuationModel;

typedef struct AUD_BackendAPI AUD_BackendAPI;
struct AUD_BackendAPI
{
	void *ctx; // Internal data store per-backend.
	
	void (*Init)(void);
	void (*Shutdown)(void);

	void (*Tick)(f32 dt, AUD_Listener listener);

	void (*Play)(AUD_SourceHandle handle);
	void (*Stop)(AUD_SourceHandle handle);
	void (*Resume)(AUD_SourceHandle handle);
	void (*Pause)(AUD_SourceHandle handle);
	void (*Reset)(AUD_SourceHandle handle);

	b32 (*IsPlaying)(AUD_SourceHandle handle);
	b32 (*IsLooping)(AUD_SourceHandle handle);

	AUD_BufferHandle (*CreateBuffer)(const void *data, u64 bytes, u32 channels, u16 sample_rate, AUD_Format format);
	void (*DestroyBuffer)(AUD_BufferHandle handle);

	AUD_SourceHandle (*CreateSourceFromBuffer)(AUD_BufferHandle handle);
	// TODO: CreateSourceFromStream
	
	void (*DestroySource)(AUD_SourceHandle handle);

	void (*SetSourceVolume)(AUD_SourceHandle handle, f32 volume);
	void (*SetSourcePitch)(AUD_SourceHandle handle, f32 pitch);
	void (*SetSourceLooping)(AUD_SourceHandle handle, b32 loop);
	void (*SetSourcePosition)(AUD_SourceHandle handle, v3 position);
	void (*SetSourceDopplerFactor)(AUD_SourceHandle handle, f32 factor);
	void (*SetSourceAttenuationModel)(AUD_SourceHandle handle, AUD_AttenuationModel model);
	void (*SetSourceAttenuationRange)(AUD_SourceHandle handle, f32 dist_min, f32 dist_max);
};

internal AUD_BackendAPI *AUD_BackendAllocAndSelect(Arena *arena);
internal void AUD_BackendSelect(AUD_BackendAPI *api);

#endif // AUDIO_BACKEND_H
