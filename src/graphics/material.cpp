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
		m_passes[type].pipeline.create(&renderInfo);

	m_passes[type].pipeline.bind(buffer);
}

void Material::render(CommandBuffer &buffer, ShaderPassType type, const RenderPass &pass)
{
	Vector<uint32_t> dynamicOffsets = {
		g_materialSystem->getGlobalBuffer()->getDynamicOffset(),
		g_materialSystem->getInstanceBuffer()->getDynamicOffset(),
		m_parameterBuffer->getDynamicOffset()
	};

	m_passes[type].pipeline.bindSet(buffer, m_passes[type].set, dynamicOffsets);

	m_passes[type].pipeline.render(buffer, pass);
}

void Material::renderMesh(CommandBuffer &buffer, ShaderPassType type, const SubMesh &mesh)
{
	RenderPass pass(mesh);
	render(buffer, type, pass);
}
