#ifndef AUDIO_BACKEND_H
#define AUDIO_BACKEND_H

#include "core/types.h"
#include "core/maths.h"
#include "audio/listener.h"

namespace aud {

struct BufferHandle {
	unsigned value = 0;

	bool operator==(const BufferHandle &other) const
	{
		return value == other.value;
	}

	bool operator!=(const BufferHandle &other) const
	{
		return value != other.value;
	}
};

struct SourceHandle {
	unsigned value = 0;

	bool operator==(const SourceHandle &other) const
	{
		return value == other.value;
	}

	bool operator!=(const SourceHandle &other) const
	{
		return value != other.value;
	}
};

enum Format {
	Format_U8,
	Format_S16,
	Format_S24,
	Format_S32,
	Format_F32,
	Format_MaxEnum
};

enum AttenuationModel {
	AttenuationModel_Inverse,
	AttenuationModel_Exponential,
	AttenuationModel_Linear,
	AttenuationModel_MaxEnum
};

class IAudioBackend {
public:
	virtual ~IAudioBackend();

	IAudioBackend(const IAudioBackend &) = delete;
	IAudioBackend &operator=(const IAudioBackend &) = delete;

	static IAudioBackend *Get();

	virtual void Tick(float dt, const Listener &listener) = 0;

	virtual void Play(SourceHandle handle) = 0;
	virtual void Stop(SourceHandle handle) = 0;
	virtual void Resume(SourceHandle handle) = 0;
	virtual void Pause(SourceHandle handle) = 0;
	virtual void Reset(SourceHandle handle) = 0;

	virtual bool IsPlaying(SourceHandle handle) = 0;
	virtual bool IsLooping(SourceHandle handle) = 0;

	virtual BufferHandle CreateBuffer(const void *data, u64 bytes, unsigned channels, unsigned sample_rate, Format format) = 0;
	virtual void DestroyBuffer(BufferHandle handle) = 0;

	virtual SourceHandle CreateSourceFromBuffer(BufferHandle handle) = 0;
	// TODO: CreateSourceFromStream
	virtual void DestroySource(SourceHandle handle) = 0;

	virtual void SetSourceVolume(SourceHandle handle, float volume) = 0;
	virtual void SetSourcePitch(SourceHandle handle, float pitch) = 0;
	virtual void SetSourcePan(SourceHandle handle, float pan) = 0; // -1 left .. 1 right
	virtual void SetSourceLooping(SourceHandle handle, bool loop) = 0;
	virtual void SetSourcePosition(SourceHandle handle, const Vec3 &position) = 0;
	virtual void SetSourceDopplerFactor(SourceHandle handle, float factor) = 0;
	virtual void SetSourceAttenuationModel(SourceHandle handle, AttenuationModel model) = 0;
	virtual void SetSourceAttenuationRange(SourceHandle handle, float dist_min, float dist_max) = 0;

protected:
	IAudioBackend();
};

}

#endif // AUDIO_BACKEND_H
