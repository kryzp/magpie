#pragma once

#include "assets.h"

namespace ast
{

struct ShaderAsset : public Asset {
	AST_DEFINE_ASSET(ASSET_TYPE_SHADER);

	ShaderAsset(const gfx::ShaderProgram &shader, gfx::Device *device)
		: shader(shader)
		, device(device)
	{
	}

	~ShaderAsset() override
	{
		if (!has_flag(ASSET_FLAG_INVALID))
			device->destroy_shader_program(shader);
	}

	gfx::ShaderProgram shader;

private:
	gfx::Device *device;
};

AssetSerializer get_shader_serializer();

}
