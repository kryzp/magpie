#include "audio_backend.h"

#include "container/vector.h"
#include "container/stack.h"

#include "ext/ma/miniaudio.h"

using namespace audio;

static u64 get_bytes_from_format(AudioFormat format)
{
	switch (format) {
		case FORMAT_U8:   return  1u;
		case FORMAT_S16:  return  2u;
		case FORMAT_S24:  return  3u;
		case FORMAT_S32:  return  4u;
		case FORMAT_F32:  return  4u;
		default:          return -1u;
	}
}

static ma_format get_ma_format(AudioFormat format)
{
	switch (format) {
		case FORMAT_U8:   return ma_format_u8;
		case FORMAT_S16:  return ma_format_s16;
		case FORMAT_S24:  return ma_format_s24;
		case FORMAT_S32:  return ma_format_s32;
		case FORMAT_F32:  return ma_format_f32;
		default:          return ma_format_unknown;
	}
}

static ma_attenuation_model get_ma_model(AttenuationModel model)
{
	switch (model) {
		case ATTENUATION_INVERSE:      return ma_attenuation_model_inverse;
		case ATTENUATION_EXPONENTIAL:  return ma_attenuation_model_exponential;
		case ATTENUATION_LINEAR:       return ma_attenuation_model_linear;
		default:                       return ma_attenuation_model_none;
	}
}

struct MiniAudioBuffer {
	bool is_valid;
	ma_audio_buffer buffer;
};

struct MiniAudioSource {
	bool is_valid;
	ma_sound sound;
};

class MiniAudioBackend : public IAudioBackend {
public:
	MiniAudioBackend();
	~MiniAudioBackend() override;

	void init() override;
	void shutdown() override;

	void tick(float dt, const AudioListener &listener) override;

	void play(AudioSourceHandle source) override;
	void stop(AudioSourceHandle source) override;
	void resume(AudioSourceHandle source) override;
	void pause(AudioSourceHandle source) override;
	void reset(AudioSourceHandle source) override;

	bool is_playing(AudioSourceHandle source) override;
	bool is_looping(AudioSourceHandle source) override;

	AudioBufferHandle create_buffer(const void *data, u64 size, u32 channels, u16 sample_rate, AudioFormat format) override;
	void destroy_buffer(AudioBufferHandle buffer) override;

	AudioSourceHandle create_source() override;
	void destroy_source(AudioSourceHandle source) override;

	void set_source_buffer(AudioSourceHandle source, AudioBufferHandle buffer) override;
	void set_source_stream(AudioSourceHandle source, const String &filepath) override;
	void set_source_volume(AudioSourceHandle source, float volume) override;
	void set_source_pitch(AudioSourceHandle source, float pitch) override;
	void set_source_looping(AudioSourceHandle source, bool loop) override;
	void set_source_position(AudioSourceHandle source, const Vec3 &position) override;
	void set_source_doppler_factor(AudioSourceHandle source, float factor) override;
	void set_source_attenuation_model(AudioSourceHandle source, AttenuationModel model) override;
	void set_source_attenuation_range(AudioSourceHandle source, float min_distance, float max_distance) override;

private:
	ma_engine engine;

	Vector<MiniAudioBuffer *> buffers;
	Vector<MiniAudioSource *> sources;

	Stack<AudioBufferHandle> free_buffer_handles;
	Stack<AudioSourceHandle> free_source_handles;
};

MiniAudioBackend::MiniAudioBackend()
	: engine()
	, buffers()
	, sources()
	, free_buffer_handles()
	, free_source_handles()
{
}

MiniAudioBackend::~MiniAudioBackend()
{
}

void MiniAudioBackend::init()
{
	ma_engine_config config = ma_engine_config_init();
	ma_engine_init(&config, &engine);
}

void MiniAudioBackend::shutdown()
{
	for (auto &source : sources) {
		if (source->is_valid) {
			ma_sound_stop(&source->sound);
			ma_sound_uninit(&source->sound);
		}
		delete source;
	}

	for (auto &buffer : buffers) {
		if (buffer->is_valid) {
			ma_audio_buffer_uninit(&buffer->buffer);
		}
		delete buffer;
	}

	ma_engine_uninit(&engine);
}

void MiniAudioBackend::tick(float dt, const AudioListener &listener)
{
	ma_engine_listener_set_position(&engine, 0,
		listener.eye.x,
		listener.eye.y,
		listener.eye.z
	);

	ma_engine_listener_set_direction(&engine, 0,
		listener.forward.x,
		listener.forward.y,
		listener.forward.z
	);

	ma_engine_listener_set_world_up(&engine, 0,
		listener.up.x,
		listener.up.y,
		listener.up.z
	);
}

void MiniAudioBackend::play(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_seek_to_pcm_frame(&sources[source]->sound, 0);
	ma_sound_start(&sources[source]->sound);
}

void MiniAudioBackend::stop(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_seek_to_pcm_frame(&sources[source]->sound, 0);
	ma_sound_stop(&sources[source]->sound);
}

void MiniAudioBackend::resume(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_start(&sources[source]->sound);
}

void MiniAudioBackend::pause(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_stop(&sources[source]->sound);
}

void MiniAudioBackend::reset(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_seek_to_pcm_frame(&sources[source]->sound, 0);
}

bool MiniAudioBackend::is_playing(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	return ma_sound_is_playing(&sources[source]->sound);
}

bool MiniAudioBackend::is_looping(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	return ma_sound_is_looping(&sources[source]->sound);
}

AudioBufferHandle MiniAudioBackend::create_buffer(const void *data, u64 size, u32 channels, u16 sample_rate, AudioFormat format)
{
	AudioBufferHandle handle;

	if (!free_buffer_handles.empty()) {
		handle = free_buffer_handles.top();
		free_buffer_handles.pop();
	} else {
		handle = buffers.size();
		buffers.push_back(new MiniAudioBuffer());
	}

	ma_format fmt = get_ma_format(format);
	u64 bytes = get_bytes_from_format(format);

	u64 frame_count = size / (channels * bytes);

	ma_audio_buffer_config config = ma_audio_buffer_config_init(fmt, channels, frame_count, data, nullptr);
	config.sampleRate = sample_rate;

	ma_result result = ma_audio_buffer_init(&config, &buffers[handle]->buffer);

	assert(result == MA_SUCCESS);

	buffers[handle]->is_valid = true;

	return handle;
}

void MiniAudioBackend::destroy_buffer(AudioBufferHandle buffer)
{
	assert(buffer != INVALID_AUDIO_BUFFER);
	
	if (buffers[buffer]->is_valid)
		ma_audio_buffer_uninit(&buffers[buffer]->buffer);

	buffers[buffer]->is_valid = false;

	free_buffer_handles.push(buffer);
}

AudioSourceHandle MiniAudioBackend::create_source()
{
	AudioSourceHandle handle;

	if (!free_source_handles.empty()) {
		handle = free_source_handles.top();
		free_source_handles.pop();
	} else {
		handle = sources.size();
		sources.push_back(new MiniAudioSource());
	}

	sources[handle]->is_valid = false;

	return handle;
}

void MiniAudioBackend::destroy_source(AudioSourceHandle source)
{
	assert(source != INVALID_AUDIO_SOURCE);

	if (sources[source]->is_valid) {
		ma_sound_uninit(&sources[source]->sound);
		sources[source]->is_valid = false;
	}

	free_source_handles.push(source);
}

void MiniAudioBackend::set_source_buffer(AudioSourceHandle source, AudioBufferHandle buffer)
{
	assert(source != INVALID_AUDIO_SOURCE);

	if (sources[source]->is_valid)
		ma_sound_uninit(&sources[source]->sound);
	
	ma_result result = ma_sound_init_from_data_source(&engine, &buffers[buffer]->buffer, 0, nullptr, &sources[source]->sound);
	assert(result == MA_SUCCESS);

	// Disable spatialization be default.
	ma_sound_set_spatialization_enabled(&sources[source]->sound, false);

	sources[source]->is_valid = true;
}

void MiniAudioBackend::set_source_stream(AudioSourceHandle source, const String &filepath)
{
	assert(source != INVALID_AUDIO_SOURCE);

	if (sources[source]->is_valid)
		ma_sound_uninit(&sources[source]->sound);
	
	ma_result result = ma_sound_init_from_file(&engine, filepath.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &sources[source]->sound);
	assert(result == MA_SUCCESS);

	// Disable spatialization be default.
	ma_sound_set_spatialization_enabled(&sources[source]->sound, false);

	sources[source]->is_valid = true;
}

void MiniAudioBackend::set_source_volume(AudioSourceHandle source, float volume)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_set_volume(&sources[source]->sound, volume);
}

void MiniAudioBackend::set_source_pitch(AudioSourceHandle source, float pitch)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_set_pitch(&sources[source]->sound, pitch);
}

void MiniAudioBackend::set_source_looping(AudioSourceHandle source, bool loop)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_set_looping(&sources[source]->sound, loop);
}

void MiniAudioBackend::set_source_position(AudioSourceHandle source, const Vec3 &position)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_set_spatialization_enabled(&sources[source]->sound, true);
	ma_sound_set_position(&sources[source]->sound, position.x, position.y, position.z);
}

void MiniAudioBackend::set_source_doppler_factor(AudioSourceHandle source, float factor)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_set_doppler_factor(&sources[source]->sound, factor);
}

void MiniAudioBackend::set_source_attenuation_model(AudioSourceHandle source, AttenuationModel model)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_attenuation_model ma_model = get_ma_model(model);

	ma_sound_set_attenuation_model(&sources[source]->sound, ma_model);
}

void MiniAudioBackend::set_source_attenuation_range(AudioSourceHandle source, float min_distance, float max_distance)
{
	assert(source != INVALID_AUDIO_SOURCE);
	assert(sources[source]->is_valid);

	ma_sound_set_min_distance(&sources[source]->sound, min_distance);
	ma_sound_set_max_distance(&sources[source]->sound, max_distance);
}

IAudioBackend *audio::get_audio_backend()
{
	static MiniAudioBackend backend;
	return &backend;
}
