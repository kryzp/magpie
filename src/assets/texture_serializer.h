#pragma once

#include "assets.h"

namespace ast
{
	class TextureAsset : public Asset {
		ASSET_DECLARE(ASSET_TYPE_TEXTURE);

	public:
		TextureAsset(const gfx::Texture *texture, gfx::Device &device)
			: texture(texture)
			, device(device)
		{
		}

		void unload() override
		{
			device.destroy_texture(texture);
		}

		const gfx::Texture *texture;

	private:
		gfx::Device &device;
	};

	IAssetSerializer *get_texture_serializer();
}
