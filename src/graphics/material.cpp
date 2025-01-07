#include "material.h"

using namespace llt;

Material::Material()
	: textures()
	, parameterBuffer()
	, passes()
{
}

Material::~Material()
{
}

uint64_t Material::getHash() const
{
	return hash::calc(passes);
}
