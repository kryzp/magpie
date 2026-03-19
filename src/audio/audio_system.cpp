#include "audio_system.h"

using namespace audio;

AudioSystem::AudioSystem()
	: backend(nullptr)
	, voice_pool()
	, free_indices()
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
	stop_all();

	backend->shutdown();
}

void AudioSystem::tick(float dt)
{
	backend->tick(dt, listener);

	for (int i = 0; i < voice_pool.size();) {
		auto &voice = voice_pool[i];

		if (voice.active && !backend->is_playing(voice.source)) {
			backend->destroy_source(voice.source);
			voice.active = false;
			free_indices.push_back(i);
		} else {
			i++;
		}
	}
}

bool AudioSystem::is_valid(const AudioHandle &handle) const
{
	if (handle.index == -1u ||
		handle.index >= voice_pool.size())
		return false;

	const auto &voice = voice_pool[handle.index];

	return voice.active && voice.handle.generation == handle.generation;
}

AudioHandle AudioSystem::allocate_voice(AudioSourceHandle source, AudioBus bus, float volume)
{
	u32 index;

	if (!free_indices.empty()) {
		index = free_indices.back();
		free_indices.pop_back();
	} else {
		index = voice_pool.size();

		voice_pool.emplace_back();
		voice_pool[index].handle = { index, 0 };
	}

	AudioVoice &voice = voice_pool[index];
	voice.active = true;
	voice.handle.generation++;

	voice.source = source;
	voice.bus = bus;
	voice.base_volume = volume;

	return voice.handle;
}

AudioHandle AudioSystem::play_sound(AudioBufferHandle buffer, AudioBus bus, float volume, float pitch)
{
	AudioSourceHandle source = backend->create_source();
	backend->set_source_buffer(source, buffer);
	backend->set_source_volume(source, get_output_volume_on_bus(bus, volume));
	backend->set_source_pitch(source, pitch);

	backend->play(source);

	return allocate_voice(source, bus, volume);
}

AudioHandle AudioSystem::play_sound_3d(AudioBufferHandle buffer, AudioBus bus, const Vec3 &position, float volume, float pitch)
{
	AudioSourceHandle source = backend->create_source();
	backend->set_source_buffer(source, buffer);
	backend->set_source_volume(source, get_output_volume_on_bus(bus, volume));
	backend->set_source_pitch(source, pitch);
	backend->set_source_position(source, position);

	backend->play(source);

	return allocate_voice(source, bus, volume);
}

void AudioSystem::stop(const AudioHandle &handle)
{
	backend->stop(voice_pool[handle.index].source);
}

void AudioSystem::stop_all()
{
	for (auto &voice : voice_pool) {
		if (!voice.active)
			continue;

		backend->stop(voice.source);
		backend->destroy_source(voice.source);

		voice.active = false;

		free_indices.push_back(voice.handle.index);
	}
}

void AudioSystem::set_sound_position(const AudioHandle &handle, const Vec3 &position)
{
	backend->set_source_position(voice_pool[handle.index].source, position);
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

	for (int bus = 0; bus < BUS_MAX_ENUM; bus++)
		update_voice_volumes((AudioBus)bus);
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
	for (auto &voice : voice_pool) {
		if (voice.active && voice.bus == bus)
			backend->set_source_volume(voice.source, get_output_volume_on_bus(bus, voice.base_volume));
	}
}
