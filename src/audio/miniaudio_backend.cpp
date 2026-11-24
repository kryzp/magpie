#include "audio/miniaudio_backend.h"

static u64 BytesFromFormat(aud::Format format)
{
	switch (format) {
		case aud::Format_U8:
			return 1u;
		case aud::Format_S16:
			return 2u;
		case aud::Format_S24:
			return 3u;
		case aud::Format_S32:
			return 4u;
		case aud::Format_F32:
			return 4u;
		default:
			AssertTrue(false);
			return 0;
	}
}

static ma_format GetMiniFormat(aud::Format format)
{
	switch (format) {
		case aud::Format_U8:
			return ma_format_u8;
		case aud::Format_S16:
			return ma_format_s16;
		case aud::Format_S24:
			return ma_format_s24;
		case aud::Format_S32:
			return ma_format_s32;
		case aud::Format_F32:
			return ma_format_f32;
		default:
			return ma_format_unknown;
	}
}

static ma_attenuation_model GetMiniAttenuationModel(aud::AttenuationModel model)
{
	switch (model) {
		case aud::AttenuationModel_Inverse:
			return ma_attenuation_model_inverse;
		case aud::AttenuationModel_Exponential:
			return ma_attenuation_model_exponential;
		case aud::AttenuationModel_Linear:
			return ma_attenuation_model_linear;
		default:
			return ma_attenuation_model_none;
	}
}

aud::MiniaudioBackend::MiniaudioBackend(LogChannel log_channel_)
	: log_channel(log_channel_)
{
	ma_engine_config config = ma_engine_config_init();
	ma_result result = ma_engine_init(&config, &engine);

	DebugLogAssert(log_channel, result == MA_SUCCESS, "Failed to initialize audio engine: %d", result);

	// reserve zero as invalid handle.
	curr_buffer_handle.value = 1;
	curr_source_handle.value = 1;

	DebugLogI(log_channel, "Initialized.");
}

aud::MiniaudioBackend::~MiniaudioBackend()
{
	for (auto &pair : sources) {
		Source *source = pair.second;

		ma_sound_stop(&source->sound);
		ma_sound_uninit(&source->sound);
		ma_audio_buffer_ref_uninit(&source->buffer_ref);

		delete source;
	}

	for (auto &pair : buffers) {
		Buffer *buffer = pair.second;

		ma_audio_buffer_uninit(&buffer->buffer);

		delete buffer;
	}

	sources.clear();
	buffers.clear();

	ma_engine_uninit(&engine);

	DebugLogI(log_channel, "Destroyed.");
}

aud::MiniaudioBackend::Buffer *aud::MiniaudioBackend::GetBuffer(BufferHandle handle)
{
	auto it = buffers.find(handle.value);
	return it != buffers.end() ? it->second : nullptr;
}

aud::MiniaudioBackend::Source *aud::MiniaudioBackend::GetSource(SourceHandle handle)
{
	auto it = sources.find(handle.value);
	return it != sources.end() ? it->second : nullptr;
}

void aud::MiniaudioBackend::Tick(float dt, const Listener &listener)
{
	ma_engine_listener_set_position(
		&engine, 0,
		listener.position.x,
		listener.position.y,
		listener.position.z
	);

	ma_engine_listener_set_direction(
		&engine, 0,
		listener.direction.x,
		listener.direction.y,
		listener.direction.z
	);

	ma_engine_listener_set_world_up(
		&engine, 0,
		0.f, 0.f, 1.f
	);
}

void aud::MiniaudioBackend::Play(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
	ma_sound_start(&source->sound);
}

void aud::MiniaudioBackend::Stop(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
	ma_sound_stop(&source->sound);
}

void aud::MiniaudioBackend::Resume(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_start(&source->sound);
}

void aud::MiniaudioBackend::Pause(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_stop(&source->sound);
}

void aud::MiniaudioBackend::Reset(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
}

bool aud::MiniaudioBackend::IsPlaying(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	return ma_sound_is_playing(&source->sound);
}

bool aud::MiniaudioBackend::IsLooping(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	return ma_sound_is_looping(&source->sound);
}

aud::BufferHandle aud::MiniaudioBackend::CreateBuffer(const void *data, u64 bytes, unsigned channels, unsigned short sample_rate, Format format)
{
	BufferHandle handle = curr_buffer_handle;
	curr_buffer_handle.value++;

	Buffer *buffer = new Buffer();
	buffers[handle.value] = buffer;

	ma_format fmt = GetMiniFormat(format);
	u64 format_bytes = BytesFromFormat(format);
	u64 frame_count = bytes / (channels * format_bytes);

	ma_audio_buffer_config config = ma_audio_buffer_config_init(fmt, channels, frame_count, data, nullptr);
	config.sampleRate = sample_rate;

	ma_result result = ma_audio_buffer_init(&config, &buffer->buffer);

	DebugLogAssert(log_channel, result == MA_SUCCESS, "Failed to create audio buffer: %d", result);

	buffer->format = fmt;
	buffer->channels = channels;
	buffer->data = data;
	buffer->frame_count = frame_count;

	return handle;
}

void aud::MiniaudioBackend::DestroyBuffer(BufferHandle handle)
{
	Buffer *buffer = GetBuffer(handle);
	DebugLogAssert(log_channel, buffer, "Buffer Handle (%u) invalid.", handle.value);

	ma_audio_buffer_uninit(&buffer->buffer);

	buffers.erase(handle.value);
	delete buffer;
}

aud::SourceHandle aud::MiniaudioBackend::CreateSourceFromBuffer(BufferHandle buffer_handle)
{
	Buffer *buffer = GetBuffer(buffer_handle);
	AssertTrue(buffer);

	SourceHandle handle = curr_source_handle;
	curr_source_handle.value++;

	Source *source = new Source();
	sources[handle.value] = source;

	ma_audio_buffer_ref_init(
		buffer->format,
		buffer->channels,
		buffer->data,
		buffer->frame_count,
		&source->buffer_ref
	);

	ma_result result = ma_sound_init_from_data_source(&engine, &source->buffer_ref, 0, nullptr, &source->sound);

	DebugLogAssert(log_channel, result == MA_SUCCESS, "Failed to create audio source: %d", result);

	// disable spatialization by default.
	ma_sound_set_spatialization_enabled(&source->sound, false);

	return handle;
}

void aud::MiniaudioBackend::DestroySource(SourceHandle handle)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_uninit(&source->sound);
	ma_audio_buffer_ref_uninit(&source->buffer_ref);

	sources.erase(handle.value);
	delete source;
}

void aud::MiniaudioBackend::SetSourceVolume(SourceHandle handle, float volume)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_volume(&source->sound, volume);
}

void aud::MiniaudioBackend::SetSourcePitch(SourceHandle handle, float pitch)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_pitch(&source->sound, pitch);
}

void aud::MiniaudioBackend::SetSourcePan(SourceHandle handle, float pan)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_pan(&source->sound, pan);
}

void aud::MiniaudioBackend::SetSourceLooping(SourceHandle handle, bool loop)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_looping(&source->sound, loop);
}

void aud::MiniaudioBackend::SetSourcePosition(SourceHandle handle, const Vec3 &position)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_spatialization_enabled(&source->sound, true);
	ma_sound_set_position(&source->sound, position.x, position.y, position.z);
}

void aud::MiniaudioBackend::SetSourceDopplerFactor(SourceHandle handle, float factor)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_doppler_factor(&source->sound, factor);
}

void aud::MiniaudioBackend::SetSourceAttenuationModel(SourceHandle handle, AttenuationModel model)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_attenuation_model mini_model = GetMiniAttenuationModel(model);
	ma_sound_set_attenuation_model(&source->sound, mini_model);
}

void aud::MiniaudioBackend::SetSourceAttenuationRange(SourceHandle handle, float dist_min, float dist_max)
{
	Source *source = GetSource(handle);
	DebugLogAssert(log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_min_distance(&source->sound, dist_min);
	ma_sound_set_max_distance(&source->sound, dist_max);
}
