
typedef struct AUD_MA_Buffer AUD_MA_Buffer;
struct AUD_MA_Buffer
{
	AUD_MA_Buffer *next;
	AUD_MA_Buffer *prev;
	
	AUD_BufferHandle handle;
	ma_audio_buffer buffer;
};

typedef struct AUD_MA_Source AUD_MA_Source;
struct AUD_MA_Source
{
	AUD_MA_Source *next;
	AUD_MA_Source *prev;
	
	AUD_SourceHandle handle;
	ma_sound sound;
};

typedef struct AUD_MA_Backend AUD_MA_Backend;
struct AUD_MA_Backend
{
	Arena *arena;
	
	ma_engine engine;

	AUD_MA_Buffer buffer_sentinel;
	AUD_MA_Buffer free_buffer_sentinel;

	AUD_MA_Source source_sentinel;
	AUD_MA_Source free_source_sentinel;

	AUD_BufferHandle curr_buffer_handle;
	AUD_SourceHandle curr_source_handle;
};

global AUD_MA_Backend *mini_backend = NULL;

internal u64
AUD_MA_BytesFromFormat(AUD_Format format)
{
	switch (format)
	{
		case AUD_Format_U8:   return 1u;
		case AUD_Format_S16:  return 2u;
		case AUD_Format_S24:  return 3u;
		case AUD_Format_S32:  return 4u;
		case AUD_Format_F32:  return 4u;
		default:              AssertTrue(false); return 0;
	}
}

internal ma_format
AUD_MA_GetMiniFormat(AUD_Format format)
{
	switch (format)
	{
		case AUD_Format_U8:   return ma_format_u8;
		case AUD_Format_S16:  return ma_format_s16;
		case AUD_Format_S24:  return ma_format_s24;
		case AUD_Format_S32:  return ma_format_s32;
		case AUD_Format_F32:  return ma_format_f32;
		default:              return ma_format_unknown;
	}
}

internal ma_attenuation_model
AUD_MA_GetMiniAttenuationModel(AUD_AttenuationModel model)
{
	switch (model)
	{
		case AUD_AttenuationModel_Inverse:      return ma_attenuation_model_inverse;
		case AUD_AttenuationModel_Exponential:  return ma_attenuation_model_exponential;
		case AUD_AttenuationModel_Linear:       return ma_attenuation_model_linear;
		default:                                return ma_attenuation_model_none;
	}
}

internal AUD_MA_Buffer *
AUD_MA_AllocBuffer(void)
{
	AUD_MA_Buffer *buffer;

	if (mini_backend->free_buffer_sentinel.next != &mini_backend->free_buffer_sentinel)
	{
		buffer = mini_backend->free_buffer_sentinel.next;
		buffer->prev->next = buffer->next;
		buffer->next->prev = buffer->prev;

		MemZeroStruct(buffer);
	}
	else
	{
		buffer = ArenaPushArray(mini_backend->arena, AUD_MA_Buffer, 1);
	}

	buffer->handle = mini_backend->curr_buffer_handle;
	mini_backend->curr_buffer_handle.value++;

	buffer->next = mini_backend->buffer_sentinel.next;
	buffer->prev = &mini_backend->buffer_sentinel;

	buffer->next->prev = buffer;
	buffer->prev->next = buffer;

	return buffer;
}

internal AUD_MA_Source *
AUD_MA_AllocSource(void)
{
	AUD_MA_Source *source;

	if (mini_backend->free_source_sentinel.next != &mini_backend->free_source_sentinel)
	{
		source = mini_backend->free_source_sentinel.next;
		source->prev->next = source->next;
		source->next->prev = source->prev;

		MemZeroStruct(source);
	}
	else
	{
		source = ArenaPushArray(mini_backend->arena, AUD_MA_Source, 1);
	}

	source->handle = mini_backend->curr_source_handle;
	mini_backend->curr_source_handle.value++;

	source->next = mini_backend->source_sentinel.next;
	source->prev = &mini_backend->source_sentinel;

	source->next->prev = source;
	source->prev->next = source;

	return source;
}

internal void
AUD_MA_ReleaseBuffer(AUD_MA_Buffer *buffer)
{
	ma_audio_buffer_uninit(&buffer->buffer);
	
	buffer->prev->next = buffer->next;
	buffer->next->prev = buffer->prev;

	buffer->next = mini_backend->free_buffer_sentinel.next;
	buffer->prev = &mini_backend->free_buffer_sentinel;

	buffer->next->prev = buffer;
	buffer->prev->next = buffer;
}

internal void
AUD_MA_ReleaseSource(AUD_MA_Source *source)
{
	ma_sound_uninit(&source->sound);

	source->prev->next = source->next;
	source->next->prev = source->prev;

	source->next = mini_backend->free_source_sentinel.next;
	source->prev = &mini_backend->free_source_sentinel;

	source->next->prev = source;
	source->prev->next = source;
}

internal AUD_MA_Buffer *
AUD_MA_GetBuffer(AUD_BufferHandle handle)
{
	AUD_MA_Buffer *sentinel = &mini_backend->buffer_sentinel;

	for (AUD_MA_Buffer *buffer = sentinel->next; buffer != sentinel; buffer = buffer->next)
	{
		if (AUD_BufferHandleMatch(handle, buffer->handle))
			return buffer;
	}

	return NULL;
}

internal AUD_MA_Source *
AUD_MA_GetSource(AUD_SourceHandle handle)
{
	AUD_MA_Source *sentinel = &mini_backend->source_sentinel;

	for (AUD_MA_Source *source = sentinel->next; source != sentinel; source = source->next)
	{
		if (AUD_SourceHandleMatch(handle, source->handle))
			return source;
	}

	return NULL;
}

internal void
AUD_MA_Init(void)
{
	ma_engine_config config = ma_engine_config_init();
	ma_engine_init(&config, &mini_backend->engine);
}

internal void
AUD_MA_Shutdown(void)
{
	AUD_MA_Source *src_sentinel = &mini_backend->source_sentinel;
	for (AUD_MA_Source *source = src_sentinel->next; source != src_sentinel; source = source->next)
	{
		ma_sound_stop(&source->sound);
		ma_sound_uninit(&source->sound);
	}

	AUD_MA_Buffer *buf_sentinel = &mini_backend->buffer_sentinel;
	for (AUD_MA_Buffer *buffer = buf_sentinel->next; buffer != buf_sentinel; buffer = buffer->next)
	{
		ma_audio_buffer_uninit(&buffer->buffer);
	}

	ma_engine_uninit(&mini_backend->engine);
}

internal void
AUD_MA_Tick(f32 dt, AUD_Listener listener)
{
	ma_engine_listener_set_position(&mini_backend->engine, 0,
									listener.position.x,
									listener.position.y,
									listener.position.z);

	ma_engine_listener_set_direction(&mini_backend->engine, 0,
									 listener.direction.x,
									 listener.direction.y,
									 listener.direction.z);

	ma_engine_listener_set_world_up(&mini_backend->engine, 0,
									0.f, 0.f, 1.f);
}

internal void
AUD_MA_Play(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
	ma_sound_start(&source->sound);
}

internal void
AUD_MA_Stop(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
	ma_sound_stop(&source->sound);
}

internal void
AUD_MA_Resume(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_start(&source->sound);
}

internal void
AUD_MA_Pause(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_stop(&source->sound);
}

internal void
AUD_MA_Reset(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_seek_to_pcm_frame(&source->sound, 0);
}

internal b32
AUD_MA_IsPlaying(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	return ma_sound_is_playing(&source->sound);
}

internal b32
AUD_MA_IsLooping(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	return ma_sound_is_looping(&source->sound);
}

internal AUD_BufferHandle
AUD_MA_CreateBuffer(const void *data, u64 bytes, u32 channels, u16 sample_rate, AUD_Format format)
{
	AUD_MA_Buffer *buffer = AUD_MA_AllocBuffer();

	ma_format fmt = AUD_MA_GetMiniFormat(format);
	u64 format_bytes = AUD_MA_BytesFromFormat(format);

	u64 frame_count = bytes / (channels * format_bytes);

	ma_audio_buffer_config config = ma_audio_buffer_config_init(fmt, channels, frame_count, data, NULL);
	config.sampleRate = sample_rate;

	ma_result result = ma_audio_buffer_init(&config, &buffer->buffer);
	AssertTrue(result == MA_SUCCESS);

	return buffer->handle;
}

internal void
AUD_MA_DestroyBuffer(AUD_BufferHandle handle)
{
	AUD_MA_Buffer *buffer = AUD_MA_GetBuffer(handle);
	AssertTrue(buffer);
	AUD_MA_ReleaseBuffer(buffer);
}

internal AUD_SourceHandle
AUD_MA_CreateSourceFromBuffer(AUD_BufferHandle handle)
{
	AUD_MA_Source *source = AUD_MA_AllocSource();
	AUD_MA_Buffer *buffer = AUD_MA_GetBuffer(handle);

	AssertTrue(buffer);

	ma_result result = ma_sound_init_from_data_source(&mini_backend->engine, &buffer->buffer, 0, NULL, &source->sound);
	AssertTrue(result == MA_SUCCESS);

	// Disable spatialization by default.
	ma_sound_set_spatialization_enabled(&source->sound, false);
	
	return source->handle;
}

internal void
AUD_MA_DestroySource(AUD_SourceHandle handle)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);
	AUD_MA_ReleaseSource(source);
}

internal void
AUD_MA_SetSourceVolume(AUD_SourceHandle handle, f32 volume)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_set_volume(&source->sound, volume);
}

internal void
AUD_MA_SetSourcePitch(AUD_SourceHandle handle, f32 pitch)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_set_pitch(&source->sound, pitch);
}

internal void
AUD_MA_SetSourceLooping(AUD_SourceHandle handle, b32 loop)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_set_looping(&source->sound, loop);
}

internal void
AUD_MA_SetSourcePosition(AUD_SourceHandle handle, v3 position)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_set_spatialization_enabled(&source->sound, true);
	ma_sound_set_position(&source->sound, position.x, position.y, position.z);
}

internal void
AUD_MA_SetSourceDopplerFactor(AUD_SourceHandle handle, f32 factor)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_set_doppler_factor(&source->sound, factor);
}

internal void
AUD_MA_SetSourceAttenuationModel(AUD_SourceHandle handle, AUD_AttenuationModel model)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_attenuation_model mini_model = AUD_MA_GetMiniAttenuationModel(model);

	ma_sound_set_attenuation_model(&source->sound, mini_model);
}

internal void
AUD_MA_SetSourceAttenuationRange(AUD_SourceHandle handle, f32 dist_min, f32 dist_max)
{
	AUD_MA_Source *source = AUD_MA_GetSource(handle);
	AssertTrue(source);

	ma_sound_set_min_distance(&source->sound, dist_min);
	ma_sound_set_max_distance(&source->sound, dist_max);
}

internal void
AUD_MA_BindAPI(AUD_BackendAPI *api)
{
	api->Init                      = AUD_MA_Init;
	api->Shutdown                  = AUD_MA_Shutdown;

	api->Tick                      = AUD_MA_Tick;

	api->Play                      = AUD_MA_Play;
	api->Stop                      = AUD_MA_Stop;
	api->Resume                    = AUD_MA_Resume;
	api->Pause                     = AUD_MA_Pause;
	api->Reset                     = AUD_MA_Reset;

	api->IsPlaying                 = AUD_MA_IsPlaying;
	api->IsLooping                 = AUD_MA_IsLooping;

	api->CreateBuffer              = AUD_MA_CreateBuffer;
	api->DestroyBuffer             = AUD_MA_DestroyBuffer;

	api->CreateSourceFromBuffer    = AUD_MA_CreateSourceFromBuffer;
	api->DestroySource             = AUD_MA_DestroySource;

	api->SetSourceVolume           = AUD_MA_SetSourceVolume;
	api->SetSourcePitch            = AUD_MA_SetSourcePitch;
	api->SetSourceLooping          = AUD_MA_SetSourceLooping;
	api->SetSourcePosition         = AUD_MA_SetSourcePosition;
	api->SetSourceDopplerFactor    = AUD_MA_SetSourceDopplerFactor;
	api->SetSourceAttenuationModel = AUD_MA_SetSourceAttenuationModel;
	api->SetSourceAttenuationRange = AUD_MA_SetSourceAttenuationRange;
}

internal AUD_BackendAPI *
AUD_BackendAllocAndSelect(Arena *arena)
{
	AUD_MA_Backend *mini = ArenaPushArray(arena, AUD_MA_Backend, 1);
	mini->arena = arena;

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
	
	AUD_BackendAPI *api = ArenaPushArray(arena, AUD_BackendAPI, 1);
	api->ctx = mini;
	
	AUD_MA_BindAPI(api);

	mini_backend = mini;
	
	return api;
}

internal void
AUD_BackendHotLoad(AUD_BackendAPI *api)
{
	mini_backend = api->ctx;
}

internal void
AUD_BackendHotUnload(AUD_BackendAPI *api)
{
}
