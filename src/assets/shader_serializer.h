#pragma once

#include "assets.h"

namespace ast
{
	class ShaderAsset : public Asset {
		ASSET_DECLARE(ASSET_TYPE_SHADER);

	public:
		ShaderAsset(const gfx::ShaderProgram *shader, gfx::Device &device)
			: shader(shader)
			, device(device)
		{
		}

		void unload() override
		{
			device.destroy_shader_program(shader);
		}

		const gfx::ShaderProgram *shader;

	private:
		gfx::Device &device;
	};

	IAssetSerializer *get_shader_serializer();
}
