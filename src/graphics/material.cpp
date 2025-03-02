#include "material.h"
#include "material_system.h"
#include "backend.h"

using namespace llt;

uint64_t Material::getHash() const
{
	uint64_t result = 0;

	for (auto& t : textures) {
		hash::combine(&result, &t);
	}

	hash::combine(&result, &vertexFormat);

	hash::combine(&result, &passes[SHADER_PASS_FORWARD].shader);
	hash::combine(&result, &passes[SHADER_PASS_SHADOW].shader);

	return result;
}

void Material::bindPipeline(CommandBuffer& buffer, RenderInfo& renderInfo, ShaderPassType type)
{
	if (this->passes[type].pipeline.getPipeline() == VK_NULL_HANDLE)
		this->passes[type].pipeline.create(&renderInfo);

	this->passes[type].pipeline.bind(buffer);
}

void Material::render(CommandBuffer& buffer, ShaderPassType type, const RenderPass& pass)
{
	uint32_t dynamicOffsets[] = {
		g_materialSystem->getGlobalBuffer()->getDynamicOffset(),
		g_materialSystem->getInstanceBuffer()->getDynamicOffset(),
		parameterBuffer->getDynamicOffset()
	};

	vkCmdBindDescriptorSets(
		buffer.getBuffer(),
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		this->passes[type].pipeline.getPipelineLayout(),
		0,
		1, &this->passes[type].set,
		LLT_ARRAY_LENGTH(dynamicOffsets), dynamicOffsets
	);

	this->passes[type].pipeline.render(buffer, pass);
}

void Material::renderMesh(CommandBuffer& buffer, ShaderPassType type, const SubMesh& mesh)
{
	RenderPass pass(mesh);
	render(buffer, type, pass);
}
