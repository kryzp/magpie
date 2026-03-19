#pragma once

#include "core/types.h"
#include "math/vec3.h"
#include "container/string.h"

#include "audio_listener.h"

namespace audio
{
	typedef u32 AudioBufferHandle;
	typedef u32 AudioSourceHandle;
	
	static constexpr AudioBufferHandle INVALID_AUDIO_BUFFER = -1u;
	static constexpr AudioSourceHandle INVALID_AUDIO_SOURCE = -1u;

	enum AudioFormat {
		FORMAT_U8,
		FORMAT_S16,
		FORMAT_S24,
		FORMAT_S32,
		FORMAT_F32,
		FORMAT_MAX_ENUM
	};

	enum AttenuationModel {
		ATTENUATION_INVERSE,
		ATTENUATION_EXPONENTIAL,
		ATTENUATION_LINEAR,
		ATTENUATION_MAX_ENUM,
	};

	class IAudioBackend {
	public:
		virtual ~IAudioBackend() = default;

		virtual void init() = 0;
		virtual void shutdown() = 0;

		virtual void tick(float dt, const AudioListener &listener) = 0;

		virtual AudioBufferHandle create_buffer(const void *data, u64 size, u32 channels, u16 sample_rate, AudioFormat format) = 0;
		virtual void destroy_buffer(AudioBufferHandle buffer) = 0;

		virtual AudioSourceHandle create_source() = 0;
		virtual void destroy_source(AudioSourceHandle source) = 0;

		virtual void set_source_buffer(AudioSourceHandle source, AudioBufferHandle buffer) = 0;	
		virtual void set_source_stream(AudioSourceHandle source, const String &filepath) = 0;
		virtual void set_source_volume(AudioSourceHandle source, float volume) = 0;
		virtual void set_source_pitch(AudioSourceHandle source, float pitch) = 0;
		virtual void set_source_looping(AudioSourceHandle source, bool loop) = 0;
		virtual void set_source_position(AudioSourceHandle source, const Vec3 &position) = 0;
		virtual void set_source_doppler_factor(AudioSourceHandle source, float factor) = 0;
		virtual void set_source_attenuation_model(AudioSourceHandle source, AttenuationModel model) = 0;
		virtual void set_source_attenuation_range(AudioSourceHandle source, float min_distance, float max_distance) = 0;

		virtual void play(AudioSourceHandle source) = 0;
		virtual void stop(AudioSourceHandle source) = 0;
		virtual void resume(AudioSourceHandle source) = 0;
		virtual void pause(AudioSourceHandle source) = 0;
		virtual void reset(AudioSourceHandle source) = 0;

		virtual bool is_playing(AudioSourceHandle source) = 0;
		virtual bool is_looping(AudioSourceHandle source) = 0;
	};

	IAudioBackend *get_audio_backend();
}
