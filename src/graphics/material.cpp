#include "material.h"
#include "material_system.h"
#include "backend.h"

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

void Material::bindPipeline(CommandBuffer &buffer, RenderInfo &renderInfo, ShaderPassType type)
{
	if (m_passes[type].pipeline.getPipeline() == VK_NULL_HANDLE)
		m_passes[type].pipeline.buildGraphicsPipeline(&renderInfo);

	buffer.bindPipeline(m_passes[type].pipeline);
}

void Material::renderMesh(CommandBuffer &buffer, ShaderPassType type, const SubMesh &mesh)
{
	Vector<uint32_t> dynamicOffsets = {
		g_materialSystem->getGlobalBuffer()->getDynamicOffset(),
		g_materialSystem->getInstanceBuffer()->getDynamicOffset(),
		m_parameterBuffer->getDynamicOffset()
	};

	buffer.bindDescriptorSets(
		m_passes[type].pipeline.getBindPoint(),
		m_passes[type].pipeline.getPipelineLayout(),
		0,
		1, &m_passes[type].set,
		dynamicOffsets.size(),
		dynamicOffsets.data()
	);

	mesh.render(buffer);
}
