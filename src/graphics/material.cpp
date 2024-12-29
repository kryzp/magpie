#include "material.h"

using namespace llt;

Material::Material()
	: textures()
	, parameterBuffer()
	, pipeline()
{
}

Material::~Material()
{
}

uint64_t Material::getHash() const
{
	uint64_t ret = 0;

	hash::combine(&ret, &pipeline);

	return ret;
}
