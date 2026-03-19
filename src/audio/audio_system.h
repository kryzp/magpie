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

	struct AudioHandle {
		u32 index = -1u;
		u32 generation = 0;
	};

	struct AudioVoice {
		AudioHandle handle;
		AudioSourceHandle source;
		AudioBus bus;
		float base_volume;
		bool active;
	};

	class AudioSystem {
	public:
		AudioSystem();
		~AudioSystem();

		void init();
		void shutdown();

		void tick(float dt);

		bool is_valid(const AudioHandle &handle) const;

		AudioHandle play_sound(AudioBufferHandle clip, AudioBus bus, float volume = 1.f, float pitch = 1.f);
		AudioHandle play_sound_3d(AudioBufferHandle clip, AudioBus bus, const Vec3 &position, float volume = 1.f, float pitch = 1.f);

		void stop(const AudioHandle &handle);
		void stop_all();

		void set_sound_position(const AudioHandle &handle, const Vec3 &position);

		const AudioListener &get_listener() const;
		void set_listener(const AudioListener &l);

		void set_master_volume(float volume);
		void set_bus_volume(AudioBus bus, float volume);

		float get_output_volume_on_bus(AudioBus bus, float base_volume) const;

	private:
		AudioHandle allocate_voice(AudioSourceHandle source, AudioBus bus, float volume);

		void update_voice_volumes(AudioBus bus);

		IAudioBackend *backend;

		Vector<AudioVoice> voice_pool;
		Vector<u32> free_indices;

		float master_volume;
		float bus_volumes[BUS_MAX_ENUM];

		AudioListener listener;
	};
}
