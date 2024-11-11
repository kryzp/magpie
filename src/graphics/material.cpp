#include "material.h"

using namespace llt;

Material::Material()
{
}

Material::~Material()
{
}

uint64_t Material::hash() const
{
	uint64_t ret = 0;
	hash::combine(&ret, &technique);
	hash::combine(&ret, &textures);
	return ret;
}

void Material::setTexture(int idx, int bindIdx, const Texture* texture, TextureSampler* sampler)
{
	textureBindIdxs[idx] = bindIdx;
	textures[idx].texture = texture;
	textures[idx].sampler = sampler;
}
