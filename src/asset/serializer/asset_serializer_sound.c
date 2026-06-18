
typedef struct AST_SoundLoadData AST_SoundLoadData;
struct AST_SoundLoadData
{
	void *pcm_data;
	u32 channels;
	u16 sample_rate;
	u64 size_in_bytes;
};

internal AST_SerializerPipelineData
AST_SoundSerializerCpu(const AST_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);

	ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
	ma_decoder decoder = {0};
	
	AST_SerializerPipelineData result = {0};
	result.stage_size = 0;

	if (ma_decoder_init_file((const char *)file_path.str, &config, &decoder) != MA_SUCCESS)
	{
		result.failed = true;
		goto end;
	}

	u64 frame_count = 0;
	ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);
	
	AST_SoundLoadData *sound_data = ArenaPushArray(load_scope, AST_SoundLoadData, 1);
	sound_data->channels = decoder.outputChannels;
	sound_data->sample_rate = decoder.outputSampleRate;
	sound_data->size_in_bytes = frame_count * sound_data->channels * sizeof(f32);
	sound_data->pcm_data = ArenaPushArray(load_scope, u8, sound_data->size_in_bytes);

	ma_decoder_read_pcm_frames(&decoder, sound_data->pcm_data, frame_count, NULL);
	ma_decoder_uninit(&decoder);

	result.data = sound_data;
	result.failed = false;
	
end:	
	ScratchRelease(&scratch);

	return result;
}

internal void
AST_SoundSerializerAlloc(const AST_Context *ctx,
						 AST_SerializerPipelineData *data,
						 AST_Asset *out,
						 Arena *arena)
{
	const AUD_BackendAPI *backend = ctx->assets->audio_backend;
	
	AST_SoundLoadData *sound_data = data->data;
	
	void *permanent_pcm = ArenaPushArray(arena, u8, sound_data->size_in_bytes);
	MemCopy(permanent_pcm, sound_data->pcm_data, sound_data->size_in_bytes);

	out->sound_data.buffer = backend->CreateBuffer(permanent_pcm,
												   sound_data->size_in_bytes,
												   sound_data->channels,
												   sound_data->sample_rate,
												   AUD_Format_F32);
}

internal void
AST_SoundSerializerReload(const AST_Context *ctx,
						  AST_SerializerPipelineData *data,
						  AST_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

internal void
AST_SoundSerializerDispose(AST_Asset *asset, AST_Assets *assets)
{
	assets->audio_backend->DestroyBuffer(asset->sound_data.buffer);
}

internal AST_Serializer
AST_GetSoundSerializer(void)
{
	static AST_Serializer sound_serializer = {
		.Cpu     = AST_SoundSerializerCpu,
		.Alloc   = AST_SoundSerializerAlloc,
		.Reload  = AST_SoundSerializerReload,
		.Gpu     = NULL,
		.End     = NULL,
		.Dispose = AST_SoundSerializerDispose,
	};

	return sound_serializer;
}
