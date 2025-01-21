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

void Material::bindPipeline(ShaderPassType type)
{
	this->passes[type].pipeline.bind();
}

void Material::render(ShaderPassType type, const RenderPass& pass)
{
	uint32_t dynamicOffsets[] = {
		g_materialSystem->getGlobalBuffer()->getDynamicOffset(),
		g_materialSystem->getInstanceBuffer()->getDynamicOffset(),
		parameterBuffer->getDynamicOffset()
	};

	vkCmdBindDescriptorSets(
		g_vulkanBackend->getGraphicsCommandBuffer(),
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		this->passes[type].pipeline.getPipelineLayout(),
		0,
		1, &this->passes[type].set,
		LLT_ARRAY_LENGTH(dynamicOffsets), dynamicOffsets
	);

	this->passes[type].pipeline.render(pass);
}

void Material::renderMesh(ShaderPassType type, const SubMesh& mesh)
{
	RenderPass pass(mesh);
	render(type, pass);
}
