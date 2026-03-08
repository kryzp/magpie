#pragma once

#include "assets.h"

#include "audio/audio_system.h"

namespace ast
{
	struct SoundAsset : public Asset {
		ASSET_DECLARE(ASSET_TYPE_SOUND);

		SoundAsset(audio::AudioBufferHandle buffer)
			: buffer(buffer)
		{
		}

		void unload() override
		{
			audio::get_audio_backend()->destroy_buffer(buffer);
			buffer = audio::INVALID_AUDIO_BUFFER;
		}

		audio::AudioBufferHandle buffer = audio::INVALID_AUDIO_BUFFER;
	};

	IAssetSerializer *get_sound_serializer();
}
