#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include "core/types.h"
#include "core/log.h"
#include "core/maths.h"
#include "audio/backend.h"
#include "audio/listener.h"

#include <list>

namespace aud {

enum Bus {
	Bus_Music,
	Bus_Sfx,
	Bus_MaxEnum
};

// null handle (value = 0) is reserved
// as the invalid handle!!
struct Handle {
	unsigned value = 0;

	bool IsValid() const
	{
		return value != 0;
	}

	bool operator==(const Handle &other) const
	{
		return value == other.value;
	}

	bool operator!=(const Handle &other) const
	{
		return value != other.value;
	}
};

struct Emitter {
	Handle handle;
	SourceHandle source;
	Bus bus = Bus_Sfx;
	float base_volume = 1.f;
	bool looping = false;
	bool paused = false;
};

struct PlayConfig {
	BufferHandle clip;
	Bus bus = Bus_Sfx;
	float volume = 1.f;
	float pitch = 1.f;
	bool spatial = false;
	Vec3 position;
	float pan = 0.f; // only for non-spatial sounds
	bool looping = false;
};

class System {
public:
	System(IAudioBackend *backend, LogChannel log_channel);
	~System();

	System(const System &) = delete;
	System &operator=(const System &) = delete;

	static System *Get();

	void Tick(float dt, const Listener &listener);

	Emitter *GetEmitter(Handle handle);

	Handle Play(const PlayConfig &config);
	void Stop(Handle handle);
	void StopAll();
	void Resume(Handle handle);
	void Pause(Handle handle);

	bool IsPlaying(Handle handle);

	void SetPositionOf(Handle handle, const Vec3 &position);
	void SetVolumeOf(Handle handle, float base_volume);
	void SetPanOf(Handle handle, float pan);

	void SetMasterVolume(float volume);
	void SetBusVolume(Bus bus, float volume);

	float GetOutputVolumeOnBus(Bus bus, float base_volume) const;
	void UpdateEmitterVolumes(Bus bus);

private:
	void ReleaseFinished();

private:
	IAudioBackend *backend;
	LogChannel log_channel;

	Listener listener;

	std::list<Emitter> emitters;
	Handle curr_emitter_handle;

	float master_volume;
	float bus_volumes[Bus_MaxEnum];
};
}

#endif // AUDIO_SYSTEM_H
