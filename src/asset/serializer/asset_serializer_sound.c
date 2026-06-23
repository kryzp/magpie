
typedef struct A_SoundLoadData A_SoundLoadData;
struct A_SoundLoadData
{
	void *pcm_data;
	u32 channels;
	u16 sample_rate;
	u64 size_in_bytes;
};

static A_SerializerPipelineData A_SoundSerializerCpu(const A_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = A_ContextSystemFilePath(ctx, scratch.arena);

	ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
	ma_decoder decoder = {0};
	
	A_SerializerPipelineData result = {0};
	result.stage_size = 0;

	if (ma_decoder_init_file((const char *)file_path.str, &config, &decoder) != MA_SUCCESS)
	{
		result.failed = true;
		goto end;
	}

	u64 frame_count = 0;
	ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);
	
	A_SoundLoadData *sound = ArenaPushArray(load_scope, A_SoundLoadData, 1);
	sound->channels = decoder.outputChannels;
	sound->sample_rate = decoder.outputSampleRate;
	sound->size_in_bytes = frame_count * sound->channels * sizeof(f32);
	sound->pcm_data = ArenaPushArray(load_scope, u8, sound->size_in_bytes);

	ma_decoder_read_pcm_frames(&decoder, sound->pcm_data, frame_count, NULL);
	ma_decoder_uninit(&decoder);

	result.data = sound;
	result.failed = false;
	
end:	
	ScratchRelease(&scratch);

	return result;
}

static void A_SoundSerializerAlloc(const A_Context *ctx,
						 A_SerializerPipelineData *data,
						 A_Asset *out,
						 Arena *arena)
{
	AU_Backend *backend = ctx->assets->audio_backend;
	
	A_SoundLoadData *sound = data->data;
	
	void *permanent_pcm = ArenaPushArray(arena, u8, sound->size_in_bytes);
	MemCopy(permanent_pcm, sound->pcm_data, sound->size_in_bytes);

	out->sound.buffer = AU_BackendCreateBuffer(backend,
													 permanent_pcm,
													 sound->size_in_bytes,
													 sound->channels,
													 sound->sample_rate,
													 AU_Format_F32);
}

static void A_SoundSerializerReload(const A_Context *ctx,
						  A_SerializerPipelineData *data,
						  A_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

static void A_SoundSerializerDispose(A_Asset *asset, A_Registry *assets)
{
	AU_BackendDestroyBuffer(assets->audio_backend, asset->sound.buffer);
}

static A_Serializer A_GetSoundSerializer(void)
{
	static A_Serializer sound_serializer = {
		.Cpu     = A_SoundSerializerCpu,
		.Alloc   = A_SoundSerializerAlloc,
		.Reload  = A_SoundSerializerReload,
		.Gpu     = NULL,
		.End     = NULL,
		.Dispose = A_SoundSerializerDispose,
	};

	return sound_serializer;
}
