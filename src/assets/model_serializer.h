#pragma once

#include "assets.h"

#include "graphics/model.h"

namespace ast
{
	class ModelAsset : public Asset {
		ASSET_DECLARE(ASSET_TYPE_MODEL);

	public:
		ModelAsset(const gfx::Model model)
			: model(model)
		{
		}

		void unload() override
		{
			for (auto &s : model.sub_models)
				s.mesh.destroy_buffers();
		}

		gfx::Model model;
	};

	IAssetSerializer *get_model_serializer();
}
