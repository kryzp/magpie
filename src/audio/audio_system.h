#pragma once

#include "container/vector.h"

#include "audio_backend.h"
#include "audio_listener.h"

namespace audio
{
	enum AudioBus {
		BUS_MUSIC,
		BUS_SFX,
		BUS_MAX_ENUM
	};

	typedef u32 AudioHandle;

	struct AudioVoice {
		AudioHandle handle;
		AudioSourceHandle source;
		AudioBus bus;
		float base_volume;
	};

	class AudioSystem {
	public:
		AudioSystem();
		~AudioSystem();

		void init();
		void shutdown();

		void tick(float dt);

		AudioHandle play_sound(AudioBufferHandle clip, AudioBus bus, float volume = 1.f, float pitch = 1.f);
		AudioHandle play_sound_3d(AudioBufferHandle clip, const Vec3 &position, AudioBus bus, float volume = 1.f, float pitch = 1.f);

		void stop(AudioHandle handle);

		void stop_all();

		const AudioListener &get_listener() const;
		void set_listener(const AudioListener &l);

		void set_master_volume(float volume);
		void set_bus_volume(AudioBus bus, float volume);

		float get_output_volume_on_bus(AudioBus bus, float base_volume) const;

	private:
		void update_voice_volumes(AudioBus bus);

		IAudioBackend *backend;

		Vector<AudioVoice> active_voices;

		float master_volume;
		float bus_volumes[BUS_MAX_ENUM];

		AudioListener listener;
	};
}
