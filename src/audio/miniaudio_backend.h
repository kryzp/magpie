#ifndef AUDIO_MINIAUDIO_BACKEND_H
#define AUDIO_MINIAUDIO_BACKEND_H

#include "core/types.h"
#include "core/log.h"
#include "core/maths.h"
#include "container/hash_map.h"
#include "audio/backend.h"

#include <miniaudio/miniaudio.h>

namespace aud {

class MiniaudioBackend final : public IAudioBackend {
public:
	explicit MiniaudioBackend(LogChannel log_channel);
	~MiniaudioBackend() override;

	void Tick(float dt, const Listener &listener) override;

	void Play(SourceHandle handle) override;
	void Stop(SourceHandle handle) override;
	void Resume(SourceHandle handle) override;
	void Pause(SourceHandle handle) override;
	void Reset(SourceHandle handle) override;

	bool IsPlaying(SourceHandle handle) override;
	bool IsLooping(SourceHandle handle) override;

	BufferHandle CreateBuffer(const void *data, u64 bytes, unsigned channels, unsigned sample_rate, Format format) override;
	void DestroyBuffer(BufferHandle handle) override;

	SourceHandle CreateSourceFromBuffer(BufferHandle handle) override;
	void DestroySource(SourceHandle handle) override;

	void SetSourceVolume(SourceHandle handle, float volume) override;
	void SetSourcePitch(SourceHandle handle, float pitch) override;
	void SetSourcePan(SourceHandle handle, float pan) override;
	void SetSourceLooping(SourceHandle handle, bool loop) override;
	void SetSourcePosition(SourceHandle handle, const Vec3 &position) override;
	void SetSourceDopplerFactor(SourceHandle handle, float factor) override;
	void SetSourceAttenuationModel(SourceHandle handle, AttenuationModel model) override;
	void SetSourceAttenuationRange(SourceHandle handle, float dist_min, float dist_max) override;

private:
	struct Buffer {
		ma_format format;
		u32 channels;
		const void *data;
		u64 frame_count;
		ma_audio_buffer buffer;
	};

	struct Source {
		ma_audio_buffer_ref buffer_ref;
		ma_sound sound;
	};

	Buffer *GetBuffer(BufferHandle handle);
	Source *GetSource(SourceHandle handle);

private:
	LogChannel log_channel;

	ma_engine engine;

	HashMap<unsigned, Buffer *> buffers;
	HashMap<unsigned, Source *> sources;

	BufferHandle curr_buffer_handle;
	SourceHandle curr_source_handle;
};

}

#endif // AUDIO_MINIAUDIO_BACKEND_H
