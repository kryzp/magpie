#include "post_process_pass.h"

#include "../render_target_mgr.h"
#include "../texture_mgr.h"
#include "../shader_mgr.h"
#include "../mesh_loader.h"

#include "vulkan/backend.h"
#include "vulkan/shader.h"
#include "vulkan/pipeline.h"
#include "vulkan/descriptor_builder.h"
#include "vulkan/descriptor_allocator.h"

using namespace llt;

void PostProcessPass::init(DescriptorPoolDynamic& pool)
{
	ShaderEffect *hdrTonemappingShader = g_shaderManager->getEffect("hdrTonemapping");

	BoundTexture targetImage;
	targetImage.texture = g_renderTargetManager->get("target")->getAttachment(0);
	targetImage.sampler = g_textureManager->getSampler("linear");

	DescriptorWriter descriptorWriter;
	descriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, targetImage.getImageInfo());

	m_hdrSet = pool.allocate(hdrTonemappingShader->getDescriptorSetLayout());
	descriptorWriter.updateSet(m_hdrSet);

	m_hdrPipeline.bindShader(hdrTonemappingShader);
	m_hdrPipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_hdrPipeline.setDepthTest(false);
	m_hdrPipeline.setDepthWrite(false);
	m_hdrPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
}

void PostProcessPass::cleanUp()
{
}

void PostProcessPass::render(CommandBuffer &buffer, GenericRenderTarget *input)
{
	if (m_hdrPipeline.getPipeline() == VK_NULL_HANDLE)
		m_hdrPipeline.buildGraphicsPipeline(buffer.getCurrentRenderInfo());

	SubMesh *quadMesh = g_meshLoader->getQuadMesh();

	buffer.bindPipeline(m_hdrPipeline);

	buffer.setShader(g_shaderManager->getEffect("hdrTonemapping"));

	buffer.bindDescriptorSets(
		0,
		1, &m_hdrSet,
		0, nullptr
	);

	quadMesh->render(buffer);
}
