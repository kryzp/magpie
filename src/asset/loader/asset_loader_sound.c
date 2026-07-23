
typedef struct A_SoundLoadData A_SoundLoadData;
struct A_SoundLoadData
{
	void *pcm_data;
	u32 channels;
	u16 sample_rate;
	u64 size_in_bytes;
};

internal A_LoadResult A_SoundLoaderLoad(const A_LCTX *ctx,
									  Arena *result_arena)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = A_GetSystemFilePath(scratch.arena, ctx->metadata.path);

	ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
	ma_decoder decoder = {0};
	
	A_LoadResult result = {0};
	result.stage_size = 0;
	result.failed = false;

	if (ma_decoder_init_file((const char *)file_path.str, &config, &decoder) != MA_SUCCESS)
	{
		result.failed = true;
		goto end;
	}

	u64 frame_count = 0;
	ma_decoder_get_length_in_pcm_frames(&decoder, (ma_uint64 *)&frame_count);
	
	A_SoundLoadData *sound = ArenaPushArray(result_arena, A_SoundLoadData, 1);
	sound->channels = decoder.outputChannels;
	sound->sample_rate = decoder.outputSampleRate;
	sound->size_in_bytes = frame_count * sound->channels * sizeof(f32);
	sound->pcm_data = ArenaPushArray(result_arena, u8, sound->size_in_bytes);

	ma_decoder_read_pcm_frames(&decoder, sound->pcm_data, frame_count, NULL);
	ma_decoder_uninit(&decoder);

	result.user_data = sound;
	
end:	
	ScratchRelease(&scratch);

	return result;
}

internal void A_SoundLoaderAlloc(const A_LCTX *ctx,
							   A_LoadResult *result,
							   Arena *asset_arena,
							   A_Asset *asset)
{
	A_SoundLoadData *sound = result->user_data;
	
	void *permanent_pcm = ArenaPushArray(asset_arena, u8, sound->size_in_bytes);
	MemCopy(permanent_pcm, sound->pcm_data, sound->size_in_bytes);

	asset->sound.buffer = AU_BackendCreateBuffer(permanent_pcm,
												 sound->size_in_bytes,
												 sound->channels,
												 sound->sample_rate,
												 AU_Format_F32);
}

internal void A_SoundLoaderDestroyAsset(A_Asset *asset)
{
	AU_BackendDestroyBuffer(asset->sound.buffer);
}

internal A_LoaderAPI A_GetSoundLoaderAPI(void)
{
	static A_LoaderAPI sound_loader_api = {
		.Load = A_SoundLoaderLoad,
		.Alloc = A_SoundLoaderAlloc,
		.UploadGPU = NULL,
		.DestroyIntermediateResources = NULL,
		.DestroyAsset = A_SoundLoaderDestroyAsset
	};

	return sound_loader_api;
}
