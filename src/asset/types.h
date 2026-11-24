#ifndef ASSET_TYPES_H
#define ASSET_TYPES_H

#include "core/types.h"
#include "core/maths.h"
#include "asset/handle.h"
#include "asset/skeleton.h"
#include "asset/material.h"
#include "asset/model.h"
#include "asset/animation.h"
#include "graphics/resource_key.h"
#include "graphics/mesh.h"
#include "audio/backend.h"
#include "script/handle.h"
#include "container/string.h"
#include "container/vector.h"

namespace ast {

struct TextureAsset {
	gfx::ResourceKey key;
	unsigned loaded_mip_count = 0;
	unsigned max_mip_count = 0;
};

struct ShaderAsset {
	gfx::ResourceKey key;
};

struct SoundAsset {
	aud::BufferHandle buffer;
	Vector<u8> pcm;
};

struct ModelAsset {
	Vector<SubModel> sub_models;
	Vector<Skeleton> skeletons;
	Vector<AnimClip> clips;
};

struct ScriptAsset {
	scr::Ref ref;
};

// only specific type is in use.
// todo: this should be a union
// but it isnt because then the constructors throw a hissy fit.
struct Asset {
	TextureAsset texture;
	ShaderAsset shader;
	SoundAsset sound;
	ModelAsset model;
	ScriptAsset script;
};

}

#endif // ASSET_TYPES_H
