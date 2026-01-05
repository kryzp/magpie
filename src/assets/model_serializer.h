#pragma once

#include "assets.h"

#include "graphics/model.h"

namespace ast
{

struct ModelAsset : public Asset {
	AST_DEFINE_ASSET(ASSET_TYPE_MODEL);

	ModelAsset()
	{
	}

	~ModelAsset() override
	{
		model.get_sub_model(0).mesh.destroy();
	}

	gfx::Model model;
};

AssetSerializer get_model_serializer();

}
