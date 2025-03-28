#include "material.h"
#include "material_system.h"

#include "vulkan/core.h"

using namespace llt;

uint64_t Material::getHash() const
{
	uint64_t result = 0;

	for (auto &t : m_textures) {
		hash::combine(&result, &t);
	}

	hash::combine(&result, &m_vertexFormat);

	// todo: dont like hashing pointers...
	hash::combine(&result, &m_passes[SHADER_PASS_FORWARD].shader);
	hash::combine(&result, &m_passes[SHADER_PASS_SHADOW].shader);

	return result;
}

const GraphicsPipelineDefinition &Material::getPipelineDef(ShaderPassType pass) const
{
	return m_passes[pass].pipeline;
}
