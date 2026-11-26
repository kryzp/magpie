#include "asset/asset_loader.h"
#include "asset/asset_system.h"
#include "audio/backend.h"

#include <miniaudio/miniaudio.h>

struct SoundLoadData : ast::LoadData {
	Vector<u8> pcm_data;
	unsigned channels = 0;
	unsigned sample_rate = 0;
};

const Vector<String> &ast::SoundLoader::GetFileExtensions() const
{
	static const Vector<String> file_extensions = {
		"mp3",
		"wav",
		"ogg",
		"flac"
	};

	return file_extensions;
}

ast::LoadResult ast::SoundLoader::Load(const LoaderContext &ctx)
{
	String file_path = AssetSystem::Get()->GetSystemFilePath(ctx.metadata.path);

	ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
	ma_decoder decoder = {};

	LoadResult result;
	result.stage_size = 0;
	result.failed = false;

	if (ma_decoder_init_file(file_path.c_str(), &config, &decoder) != MA_SUCCESS) {
		result.failed = true;
		return result;
	}

	ma_uint64 frame_count = 0;
	ma_decoder_get_length_in_pcm_frames(&decoder, &frame_count);

	SoundLoadData *sound = new SoundLoadData();
	sound->channels = decoder.outputChannels;
	sound->sample_rate = decoder.outputSampleRate;
	sound->pcm_data.resize(frame_count * sound->channels * sizeof(float));

	ma_decoder_read_pcm_frames(&decoder, sound->pcm_data.data(), frame_count, nullptr);
	ma_decoder_uninit(&decoder);

	result.user_data = sound;

	return result;
}

void ast::SoundLoader::Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset)
{
	SoundLoadData *sound = static_cast<SoundLoadData *>(result->user_data);

	asset->sound.pcm = sound->pcm_data;

	asset->sound.buffer = aud::IAudioBackend::Get()->CreateBuffer(
		asset->sound.pcm.data(),
		asset->sound.pcm.size(),
		sound->channels,
		sound->sample_rate,
		aud::Format_F32
	);
}

void ast::SoundLoader::DestroyAsset(Asset *asset)
{
	aud::IAudioBackend::Get()->DestroyBuffer(asset->sound.buffer);
}
