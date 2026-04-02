#ifndef AUDIO_BACKEND_H
#define AUDIO_BACKEND_H

typedef u32 AUD_BufferHandle;
typedef u32 AUD_SourceHandle;

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

typedef struct AUD_Backend AUD_Backend;
struct AUD_Backend
{
	void (*Init)(void);
	void (*Shutdown)(void);

	void (*Tick)(f32 dt, AUD_Listener listener);

	void (*Play)(AUD_SourceHandle source);
	void (*Stop)(AUD_SourceHandle source);
	void (*Resume)(AUD_SourceHandle source);
	void (*Pause)(AUD_SourceHandle source);
	void (*Reset)(AUD_SourceHandle source);

	b32 (*IsPlaying)(AUD_SourceHandle source);
	b32 (*IsLooping)(AUD_SourceHandle source);

	AUD_BufferHandle (*CreateBuffer)(const void *data, u64 bytes, u32 channels, u16 sample_rate, AUD_Format format);
	void (*DestroyBuffer)(AUD_BufferHandle buffer);

	AUD_SourceHandle (*CreateSource)(void);
	void (*DestroySource)(AUD_SourceHandle source);

	void (*SetSourceBuffer)(AUD_SourceHandle source, AUD_BufferHandle buffer);
	void (*SetSourceStream)(AUD_SourceHandle source, String8 path);
	void (*SetSourceVolume)(AUD_SourceHandle source, f32 volume);
	void (*SetSourcePitch)(AUD_SourceHandle source, f32 pitch);
	void (*SetSourceLooping)(AUD_SourceHandle source, b32 loop);
	void (*SetSourcePosition)(AUD_SourceHandle source, v3 position);
	void (*SetSourceDopplerFactor)(AUD_SourceHandle source, f32 factor);
	void (*SetSourceAttenuationModel)(AUD_SourceHandle source, AUD_AttenuationModel model);
	void (*SetSourceAttenuationRange)(AUD_SourceHandle source, f32 dist_min, f32 dist_max);
};

internal AUD_Backend *AUD_BackendAlloc(Arena *arena);
internal void AUD_BackendFree(Arena *arena);

#endif // AUDIO_BACKEND_H
