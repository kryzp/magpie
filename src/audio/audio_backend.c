
static AU_Backend *au_backend = NULL;

typedef struct AU_MA_Buffer AU_MA_Buffer;
struct AU_MA_Buffer
{
	AU_MA_Buffer *next;
	AU_MA_Buffer *prev;

	ma_format format;
	u32 channels;
	const void *data;
	u64 frame_count;
	
	AU_BufferHandle handle;
	ma_audio_buffer buffer;
};

typedef struct AU_MA_Source AU_MA_Source;
struct AU_MA_Source
{
	AU_MA_Source *next;
	AU_MA_Source *prev;
	
	ma_audio_buffer_ref buffer_ref;
	
	AU_SourceHandle handle;
	ma_sound sound;
};

typedef struct AU_Backend AU_Backend;
struct AU_Backend
{
	Arena *arena;

	LOG_Channel log_channel;
	
	ma_engine engine;

	AU_MA_Buffer buffer_sentinel;
	AU_MA_Buffer free_buffer_sentinel;

	AU_MA_Source source_sentinel;
	AU_MA_Source free_source_sentinel;

	AU_BufferHandle curr_buffer_handle;
	AU_SourceHandle curr_source_handle;
};

static u64 AU_MA_BytesFromFormat(AU_Format format)
{
	switch (format)
	{
		case AU_Format_U8:   return 1u;
		case AU_Format_S16:  return 2u;
		case AU_Format_S24:  return 3u;
		case AU_Format_S32:  return 4u;
		case AU_Format_F32:  return 4u;
		default:             AssertTrue(false); return 0;
	}
}

static ma_format AU_MA_GetMiniFormat(AU_Format format)
{
	switch (format)
	{
		case AU_Format_U8:   return ma_format_u8;
		case AU_Format_S16:  return ma_format_s16;
		case AU_Format_S24:  return ma_format_s24;
		case AU_Format_S32:  return ma_format_s32;
		case AU_Format_F32:  return ma_format_f32;
		default:             return ma_format_unknown;
	}
}

static ma_attenuation_model AU_MA_GetMiniAttenuationModel(AU_AttenuationModel model)
{
	switch (model)
	{
		case AU_AttenuationModel_Inverse:      return ma_attenuation_model_inverse;
		case AU_AttenuationModel_Exponential:  return ma_attenuation_model_exponential;
		case AU_AttenuationModel_Linear:       return ma_attenuation_model_linear;
		default:                               return ma_attenuation_model_none;
	}
}

static AU_MA_Buffer *AU_MA_AllocBuffer(void)
{
	AU_MA_Buffer *buffer;

	if (au_backend->free_buffer_sentinel.next != &au_backend->free_buffer_sentinel)
	{
		buffer = au_backend->free_buffer_sentinel.next;
		buffer->prev->next = buffer->next;
		buffer->next->prev = buffer->prev;

		MemZeroStruct(buffer);
	}
	else
	{
		buffer = ArenaPushArray(au_backend->arena, AU_MA_Buffer, 1);
	}
	
	buffer->handle = au_backend->curr_buffer_handle;
	au_backend->curr_buffer_handle.value++;

	buffer->next = au_backend->buffer_sentinel.next;
	buffer->prev = &au_backend->buffer_sentinel;

	buffer->next->prev = buffer;
	buffer->prev->next = buffer;

	return buffer;
}

static AU_MA_Source *AU_MA_AllocSource(void)
{
	AU_MA_Source *source;

	if (au_backend->free_source_sentinel.next != &au_backend->free_source_sentinel)
	{
		source = au_backend->free_source_sentinel.next;
		source->prev->next = source->next;
		source->next->prev = source->prev;

		MemZeroStruct(source);
	}
	else
	{
		source = ArenaPushArray(au_backend->arena, AU_MA_Source, 1);
	}

	source->handle = au_backend->curr_source_handle;
	au_backend->curr_source_handle.value++;

	source->next = au_backend->source_sentinel.next;
	source->prev = &au_backend->source_sentinel;

	source->next->prev = source;
	source->prev->next = source;

	return source;
}

static void AU_MA_ReleaseBuffer(AU_MA_Buffer *buffer)
{
	ma_audio_buffer_uninit(&buffer->buffer);
	
	buffer->prev->next = buffer->next;
	buffer->next->prev = buffer->prev;

	buffer->next = au_backend->free_buffer_sentinel.next;
	buffer->prev = &au_backend->free_buffer_sentinel;

	buffer->next->prev = buffer;
	buffer->prev->next = buffer;
}

static void AU_MA_ReleaseSource(AU_MA_Source *source)
{
	ma_sound_uninit(&source->sound);
	ma_audio_buffer_ref_uninit(&source->buffer_ref);
	
	source->prev->next = source->next;
	source->next->prev = source->prev;

	source->next = au_backend->free_source_sentinel.next;
	source->prev = &au_backend->free_source_sentinel;

	source->next->prev = source;
	source->prev->next = source;
}

static AU_MA_Buffer *AU_MA_GetBuffer(AU_BufferHandle handle)
{
	AU_MA_Buffer *sentinel = &au_backend->buffer_sentinel;

	for (AU_MA_Buffer *buffer = sentinel->next; buffer != sentinel; buffer = buffer->next)
	{
		if (AU_BufferHandleMatch(handle, buffer->handle))
			return buffer;
	}

	return NULL;
}

static AU_MA_Source *AU_MA_GetSource(AU_SourceHandle handle)
{
	AU_MA_Source *sentinel = &au_backend->source_sentinel;

	for (AU_MA_Source *source = sentinel->next; source != sentinel; source = source->next)
	{
		if (AU_SourceHandleMatch(handle, source->handle))
			return source;
	}

	return NULL;
}

static AU_Backend *AU_BackendAllocAndSelect(Arena *arena, LOG_Channel log_channel)
{
	AU_Backend *mini = ArenaPushArray(arena, AU_Backend, 1);
	mini->arena = arena;
	mini->log_channel = log_channel;

	ma_engine_config config = ma_engine_config_init();
	ma_engine_init(&config, &mini->engine);

	// reserve zero as invalid hnadle.
	mini->curr_buffer_handle.value = 1;
	mini->curr_source_handle.value = 1;

	mini->buffer_sentinel.next = &mini->buffer_sentinel;
	mini->buffer_sentinel.prev = &mini->buffer_sentinel;
	mini->free_buffer_sentinel.next = &mini->free_buffer_sentinel;
	mini->free_buffer_sentinel.prev = &mini->free_buffer_sentinel;

	mini->source_sentinel.next = &mini->source_sentinel;
	mini->source_sentinel.prev = &mini->source_sentinel;
	mini->free_source_sentinel.next = &mini->free_source_sentinel;
	mini->free_source_sentinel.prev = &mini->free_source_sentinel;

	AU_BackendSelectContext(mini);

	DebugLogI(mini->log_channel, "Initialized.");
	
	return mini;
}

static void AU_BackendShutdown(void)
{
	AU_MA_Source *src_sentinel = &au_backend->source_sentinel;
	for (AU_MA_Source *source = src_sentinel->next; source != src_sentinel; source = source->next)
	{
		ma_sound_stop(&source->sound);
		ma_sound_uninit(&source->sound);
	}

	AU_MA_Buffer *buf_sentinel = &au_backend->buffer_sentinel;
	for (AU_MA_Buffer *buffer = buf_sentinel->next; buffer != buf_sentinel; buffer = buffer->next)
	{
		ma_audio_buffer_uninit(&buffer->buffer);
	}

	ma_engine_uninit(&au_backend->engine);
	
	DebugLogI(au_backend->log_channel, "Destroyed.");

	au_backend = NULL;
}

static void AU_BackendSelectContext(AU_Backend *backend)
{
	au_backend = backend;
}

static void AU_BackendTick(f32 dt, AU_Listener listener)
{
	ma_engine_listener_set_position(&au_backend->engine, 0,
									listener.position.x,
									listener.position.y,
									listener.position.z);

	ma_engine_listener_set_direction(&au_backend->engine, 0,
									 listener.direction.x,
									 listener.direction.y,
									 listener.direction.z);

	ma_engine_listener_set_world_up(&au_backend->engine, 0,
									0.f, 0.f, 1.f);
}

static void AU_BackendPlay(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
	ma_sound_start(&source->sound);
}

static void AU_BackendStop(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
	ma_sound_stop(&source->sound);
}

static void AU_BackendResume(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_start(&source->sound);
}

static void AU_BackendPause(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_stop(&source->sound);
}

static void AU_BackendReset(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
}

static b32 AU_BackendIsPlaying(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	return ma_sound_is_playing(&source->sound);
}

static b32 AU_BackendIsLooping(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	return ma_sound_is_looping(&source->sound);
}

static AU_BufferHandle AU_BackendCreateBuffer(const void *data, u64 bytes, u32 channels, u16 sample_rate, AU_Format format)
{
	AU_MA_Buffer *buffer = AU_MA_AllocBuffer();

	ma_format fmt = AU_MA_GetMiniFormat(format);
	u64 format_bytes = AU_MA_BytesFromFormat(format);

	u64 frame_count = bytes / (channels * format_bytes);

	ma_audio_buffer_config config = ma_audio_buffer_config_init(fmt, channels, frame_count, data, NULL);
	config.sampleRate = sample_rate;

	ma_result result = ma_audio_buffer_init(&config, &buffer->buffer);
	DebugLogAssert(au_backend->log_channel, result == MA_SUCCESS, "Failed to create audio buffer: %d", result);

	buffer->format = fmt;
	buffer->channels = channels;
	buffer->data = data;
	buffer->frame_count = frame_count;
	
	return buffer->handle;
}

static void AU_BackendDestroyBuffer(AU_BufferHandle handle)
{
	AU_MA_Buffer *buffer = AU_MA_GetBuffer(handle);
	DebugLogAssert(au_backend->log_channel, buffer, "Buffer Handle (%u) invalid.", handle.value);

	AU_MA_ReleaseBuffer(buffer);
}

static AU_SourceHandle AU_BackendCreateSourceFromBuffer(AU_BufferHandle handle)
{
	AU_MA_Source *source = AU_MA_AllocSource();
	AU_MA_Buffer *buffer = AU_MA_GetBuffer(handle);

	AssertTrue(buffer);

	ma_audio_buffer_ref_init(buffer->format,
							 buffer->channels,
							 buffer->data,
							 buffer->frame_count,
							 &source->buffer_ref);

	ma_result result = ma_sound_init_from_data_source(&au_backend->engine, &source->buffer_ref, 0, NULL, &source->sound);
	DebugLogAssert(au_backend->log_channel, result == MA_SUCCESS, "Failed to create audio source: %d", result);
	
	// Disable spatialization by default.
	ma_sound_set_spatialization_enabled(&source->sound, false);
	
	return source->handle;
}

static void AU_BackendDestroySource(AU_SourceHandle handle)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);
	
	AU_MA_ReleaseSource(source);
}

static void AU_BackendSetSourceVolume(AU_SourceHandle handle, f32 volume)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_volume(&source->sound, volume);
}

static void AU_BackendSetSourcePitch(AU_SourceHandle handle, f32 pitch)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_pitch(&source->sound, pitch);
}

static void AU_BackendSetSourceLooping(AU_SourceHandle handle, b32 loop)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_looping(&source->sound, loop);
}

static void AU_BackendSetSourcePosition(AU_SourceHandle handle, v3 position)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_spatialization_enabled(&source->sound, true);
	ma_sound_set_position(&source->sound, position.x, position.y, position.z);
}

static void AU_BackendSetSourceDopplerFactor(AU_SourceHandle handle, f32 factor)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_doppler_factor(&source->sound, factor);
}

static void AU_BackendSetSourceAttenuationModel(AU_SourceHandle handle, AU_AttenuationModel model)
{
	AU_MA_Source *source = AU_MA_GetSource(handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_attenuation_model mini_model = AU_MA_GetMiniAttenuationModel(model);

	ma_sound_set_attenuation_model(&source->sound, mini_model);
}

static void AU_BackendSetSourceAttenuationRange(AU_SourceHandle handle, f32 dist_min, f32 dist_max)
{
	AU_MA_Source *source = AU_MA_GetSource( handle);
	DebugLogAssert(au_backend->log_channel, source, "Source Handle (%u) invalid.", handle.value);

	ma_sound_set_min_distance(&source->sound, dist_min);
	ma_sound_set_max_distance(&source->sound, dist_max);
}
