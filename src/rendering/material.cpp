#include "material.h"
#include "material_system.h"

#include "vulkan/backend.h"

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

GraphicsPipelineDefinition &Material::getPipelineDef(ShaderPassType pass)
{
	return m_passes[pass].pipeline;
}

void Material::bindDescriptorSets(CommandBuffer &cmd, ShaderPassType pass, VkPipelineLayout layout)
{
	Vector<uint32_t> dynamicOffsets = {
		g_materialSystem->getGlobalBuffer()->getDynamicOffset(),
		g_materialSystem->getInstanceBuffer()->getDynamicOffset(),
		m_parameterBuffer->getDynamicOffset()
	};

	cmd.bindDescriptorSets(
		0,
		layout,
		{ m_passes[pass].set },
		dynamicOffsets
	);
}
