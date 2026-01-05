#pragma once

#include "assets.h"

namespace ast
{

struct TextureAsset : public Asset {
	AST_DEFINE_ASSET(ASSET_TYPE_TEXTURE);

	TextureAsset(const gfx::Texture &texture, gfx::Device *device)
		: texture(texture)
		, device(device)
	{
	}

	~TextureAsset() override
	{
		if (!has_flag(ASSET_FLAG_INVALID))
			device->destroy_texture(texture);
	}

	gfx::Texture texture;

private:
	gfx::Device *device;
};

AssetSerializer get_texture_serializer();

}
