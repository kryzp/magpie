#include "audio_system.h"

using namespace audio;

AudioSystem::AudioSystem()
	: backend(nullptr)
	, active_voices()
	, master_volume()
	, bus_volumes{}
	, listener()
{
}

AudioSystem::~AudioSystem()
{
}

void AudioSystem::init()
{
	this->backend = get_audio_backend();
	backend->init();

	master_volume = 1.f;

	for (int i = 0; i < BUS_MAX_ENUM; i++)
		bus_volumes[i] = 1.f;
}

void AudioSystem::shutdown()
{
	backend->shutdown();
}

void AudioSystem::tick(float dt)
{
	backend->tick(dt, listener);

	for (int i = 0; i < active_voices.size();) {
		auto &voice = active_voices[i];

		if (!backend->is_playing(voice.source)) {
			backend->destroy_source(voice.source);
			active_voices.erase(active_voices.begin() + i);
		} else {
			i++;
		}
	}
}

void AudioSystem::play_sound(AudioBufferHandle buffer, AudioBus bus, float volume, float pitch)
{
	AudioSourceHandle source = backend->create_source();
	backend->set_source_buffer(source, buffer);
	backend->set_source_volume(source, get_output_volume_on_bus(bus, volume));
	backend->set_source_pitch(source, pitch);

	backend->play(source);

	AudioVoice voice = {};
	voice.source = source;
	voice.bus = bus;
	voice.base_volume = volume;

	active_voices.push_back(voice);
}

void AudioSystem::play_sound_3d(AudioBufferHandle buffer, const Vec3 &position, AudioBus bus, float volume, float pitch)
{
	AudioSourceHandle source = backend->create_source();
	backend->set_source_buffer(source, buffer);
	backend->set_source_volume(source, get_output_volume_on_bus(bus, volume));
	backend->set_source_pitch(source, pitch);
	backend->set_source_position(source, position);

	backend->play(source);

	AudioVoice voice = {};
	voice.source = source;
	voice.bus = bus;
	voice.base_volume = volume;

	active_voices.push_back(voice);
}

const AudioListener &AudioSystem::get_listener() const
{
	return listener;
}

void AudioSystem::set_listener(const AudioListener &l)
{
	this->listener = l;
}

void AudioSystem::set_master_volume(float volume)
{
	master_volume = volume;
}

float AudioSystem::get_output_volume_on_bus(AudioBus bus, float base_volume) const
{
	return base_volume * bus_volumes[bus] * master_volume;
}

void AudioSystem::set_bus_volume(AudioBus bus, float volume)
{
	bus_volumes[bus] = volume;
	update_voice_volumes(bus);
}

void AudioSystem::update_voice_volumes(AudioBus bus)
{
	for (auto &voice : active_voices) {
		if (voice.bus == bus)
			backend->set_source_volume(voice.source, get_output_volume_on_bus(bus, voice.base_volume));
	}
}
