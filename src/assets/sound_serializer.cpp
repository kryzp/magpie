#include "sound_serializer.h"

#include "ext/miniaudio.h"

using namespace ast;

struct SoundLoadData {
	void *pcm_data;
	u32 channels;
	u16 sample_rate;
	u64 size_in_bytes;
};

class SoundSerializer : public IAssetSerializer {
public:
	AssetLoadResult load(const AssetLoadContext &ctx) override;
	Asset *finalize(const AssetLoadContext &ctx, const AssetLoadResult &result, Asset *existing_asset, gfx::Device &device) override;
};

AssetLoadResult SoundSerializer::load(const AssetLoadContext &ctx)
{
	String system_file_path = ctx.system_file_path();

	ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
	ma_decoder decoder;
	
	AssetLoadResult result = {};

	if (ma_decoder_init_file(system_file_path.c_str(), &config, &decoder) != MA_SUCCESS) {
		result.failed = true;
		return result;
	}

	u64 frame_count;
	ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);
	
	SoundLoadData *load_data = ctx.arena.push<SoundLoadData>();

	load_data->channels = decoder.outputChannels;
	load_data->sample_rate = decoder.outputSampleRate;
	load_data->size_in_bytes = frame_count * load_data->channels * sizeof(float);
	load_data->pcm_data = ctx.arena.push_bytes(load_data->size_in_bytes);

	ma_decoder_read_pcm_frames(&decoder, load_data->pcm_data, frame_count, nullptr);
	ma_decoder_uninit(&decoder);

	result.data = load_data;
	result.stage_size = 0;
	result.failed = false;

	return result;
}

Asset *SoundSerializer::finalize(
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	Asset *existing_asset,
	gfx::Device &device
)
{
	SoundLoadData *load_data = (SoundLoadData *)result.data;

	void *permanent_pcm = ctx.arena.push_bytes(load_data->size_in_bytes);

	memcpy(permanent_pcm, load_data->pcm_data, load_data->size_in_bytes);

	audio::AudioBufferHandle new_buffer = audio::get_audio_backend()->create_buffer(
		permanent_pcm,
		load_data->size_in_bytes,
		load_data->channels,
		load_data->sample_rate,
		audio::FORMAT_F32
	);

	if (existing_asset) {
		SoundAsset *sound_asset = existing_asset->as<SoundAsset>();
		audio::get_audio_backend()->destroy_buffer(sound_asset->buffer);
		sound_asset->buffer = new_buffer;
		return sound_asset;
	}

	return ctx.arena.push<SoundAsset>(new_buffer);
}

IAssetSerializer *ast::get_sound_serializer()
{
	static SoundSerializer sound_serializer;
	return &sound_serializer;
}
