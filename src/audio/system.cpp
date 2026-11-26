#include "audio/system.h"

static aud::System *selected_system = nullptr;

aud::System *aud::System::Get()
{
	return selected_system;
}

aud::System::System(IAudioBackend *backend_, LogChannel log_channel_)
	: backend(backend_)
	, log_channel(log_channel_)
	, master_volume(1.f)
{
	for (int b = 0; b < Bus_MaxEnum; b++)
		bus_volumes[b] = 1.f;

	curr_emitter_handle.value = 1;

	selected_system = this;

	DebugLogI(log_channel, "Initialized.");
}

aud::System::~System()
{
	StopAll();

	DebugLogI(log_channel, "Destroyed.");

	if (selected_system == this)
		selected_system = nullptr;
}

void aud::System::Tick(float dt, const Listener &listener_)
{
	listener = listener_;

	ReleaseFinished();
}

void aud::System::ReleaseFinished()
{
	auto it = emitters.begin();

	while (it != emitters.end()) {
		bool finished = !it->looping && !it->paused && !backend->IsPlaying(it->source);

		if (!finished) {
			++it;
			continue;
		}

		backend->DestroySource(it->source);
		it = emitters.erase(it);
	}
}

aud::Emitter *aud::System::GetEmitter(Handle handle)
{
	for (Emitter &emitter : emitters) {
		if (emitter.handle == handle)
			return &emitter;
	}

	return nullptr;
}

aud::Handle aud::System::Play(const PlayConfig &config)
{
	SourceHandle source = backend->CreateSourceFromBuffer(config.clip);

	backend->SetSourceVolume(source, GetOutputVolumeOnBus(config.bus, config.volume));
	backend->SetSourcePitch(source, config.pitch);

	backend->SetSourceLooping(source, config.looping);

	if (config.spatial)
		backend->SetSourcePosition(source, config.position);
	else
		backend->SetSourcePan(source, config.pan);

	backend->Play(source);

	Emitter emitter;
	emitter.handle = curr_emitter_handle;
	emitter.source = source;
	emitter.bus = config.bus;
	emitter.base_volume = config.volume;
	emitter.looping = config.looping;

	curr_emitter_handle.value++;

	emitters.push_front(emitter);

	return emitter.handle;
}

void aud::System::Stop(Handle handle)
{
	for (auto it = emitters.begin(); it != emitters.end(); ++it) {
		if (it->handle != handle)
			continue;

		backend->Stop(it->source);
		backend->DestroySource(it->source);

		emitters.erase(it);
		return;
	}

	DebugLogAssert(log_channel, false, "Invalid handle.");
}

void aud::System::StopAll()
{
	for (Emitter &emitter : emitters) {
		backend->Stop(emitter.source);
		backend->DestroySource(emitter.source);
	}

	emitters.clear();
}

void aud::System::Resume(Handle handle)
{
	Emitter *emitter = GetEmitter(handle);

	DebugLogAssert(log_channel, emitter, "Invalid handle.");

	emitter->paused = false;
	backend->Resume(emitter->source);
}

void aud::System::Pause(Handle handle)
{
	Emitter *emitter = GetEmitter(handle);

	DebugLogAssert(log_channel, emitter, "Invalid handle.");

	emitter->paused = true;
	backend->Pause(emitter->source);
}

bool aud::System::IsPlaying(Handle handle)
{
	Emitter *emitter = GetEmitter(handle);

	if (!emitter)
		return false;

	return backend->IsPlaying(emitter->source);
}

void aud::System::SetPositionOf(Handle handle, const Vec3 &position)
{
	Emitter *emitter = GetEmitter(handle);

	DebugLogAssert(log_channel, emitter, "Invalid handle.");

	backend->SetSourcePosition(emitter->source, position);
}

void aud::System::SetVolumeOf(Handle handle, float base_volume)
{
	Emitter *emitter = GetEmitter(handle);
	DebugLogAssert(log_channel, emitter, "Invalid handle.");

	emitter->base_volume = base_volume;
	backend->SetSourceVolume(emitter->source, GetOutputVolumeOnBus(emitter->bus, base_volume));
}

void aud::System::SetPanOf(Handle handle, float pan)
{
	Emitter *emitter = GetEmitter(handle);
	DebugLogAssert(log_channel, emitter, "Invalid handle.");

	backend->SetSourcePan(emitter->source, pan);
}

void aud::System::SetMasterVolume(float volume)
{
	master_volume = volume;

	for (int b = 0; b < Bus_MaxEnum; b++)
		UpdateEmitterVolumes(static_cast<Bus>(b));
}

void aud::System::SetBusVolume(Bus bus, float volume)
{
	bus_volumes[bus] = volume;
	UpdateEmitterVolumes(bus);
}

float aud::System::GetOutputVolumeOnBus(Bus bus, float base_volume) const
{
	return base_volume * bus_volumes[bus] * master_volume;
}

void aud::System::UpdateEmitterVolumes(Bus bus)
{
	for (Emitter &emitter : emitters) {
		if (emitter.bus == bus)
			backend->SetSourceVolume(emitter.source, GetOutputVolumeOnBus(bus, emitter.base_volume));
	}
}
