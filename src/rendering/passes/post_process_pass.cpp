#include "post_process_pass.h"

#include "../render_target_mgr.h"
#include "../texture_mgr.h"
#include "../shader_mgr.h"
#include "../mesh_loader.h"

#include "math/colour.h"

#include "vulkan/core.h"
#include "vulkan/shader.h"
#include "vulkan/pipeline.h"
#include "vulkan/descriptor_builder.h"
#include "vulkan/descriptor_allocator.h"

llt::PostProcessPass llt::g_postProcessPass;

using namespace llt;

void PostProcessPass::init(DescriptorPoolDynamic &pool, RenderTarget *input)
{
	initDefaultValues();

	createBloomResources(pool, input);
	createHDRResources(pool, input);
}

void PostProcessPass::dispose()
{
}

void PostProcessPass::initDefaultValues()
{
	m_exposure = 2.0f;

	m_bloomRadius = 0.0f;
	m_bloomIntensity = 0.0f;
}

void PostProcessPass::createHDRResources(DescriptorPoolDynamic &pool, RenderTarget *input)
{
	BoundTexture sceneTexture(
		input->getAttachment(0),
		g_textureManager->getSampler("linear")
	);

	BoundTexture bloomTexture(
		m_bloomTarget->getAttachment(0),
		g_textureManager->getSampler("linear")
	);

	ShaderEffect *hdrTonemappingShader = g_shaderManager->getEffect("hdr_tonemapping");

	m_hdrSet = pool.allocate(hdrTonemappingShader->getDescriptorSetLayout());

	DescriptorWriter()
		.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sceneTexture.getStandardImageInfo())
		.writeImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, bloomTexture.getStandardImageInfo())
		.updateSet(m_hdrSet);

	m_hdrPipeline.setShader(hdrTonemappingShader);
	m_hdrPipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_hdrPipeline.setDepthTest(false);
	m_hdrPipeline.setDepthWrite(false);
}

void PostProcessPass::createBloomResources(DescriptorPoolDynamic &pool, RenderTarget *input)
{
	m_bloomTarget = g_renderTargetManager->createTarget(
		"bloomTarget",
		g_vkCore->m_swapchain->getWidth(),
		g_vkCore->m_swapchain->getHeight(),
		{
			VK_FORMAT_R32G32B32A32_SFLOAT
		},
		VK_SAMPLE_COUNT_1_BIT,
		BLOOM_MIPS
	);

	// ---

	BoundTexture downsampleInputImage(
		input->getAttachment(0),
		g_textureManager->getSampler("linear")
	);

	ShaderEffect *bloomDownsampleShader = g_shaderManager->getEffect("bloom_downsample");

	m_bloomDownsampleSet = pool.allocate(bloomDownsampleShader->getDescriptorSetLayout());

	DescriptorWriter()
		.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, downsampleInputImage.getStandardImageInfo())
		.updateSet(m_bloomDownsampleSet);

	m_bloomDownsamplePipeline.setShader(bloomDownsampleShader);
	m_bloomDownsamplePipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_bloomDownsamplePipeline.setDepthTest(false);
	m_bloomDownsamplePipeline.setDepthWrite(false);

	// ---

	ShaderEffect *bloomUpsampleShader = g_shaderManager->getEffect("bloom_upsample");

	BlendState upsampleBlend;
	upsampleBlend.enabled = true;
	upsampleBlend.colour.op = VK_BLEND_OP_ADD;
	upsampleBlend.colour.src = VK_BLEND_FACTOR_ONE;
	upsampleBlend.colour.dst = VK_BLEND_FACTOR_ONE;
	upsampleBlend.alpha.op = VK_BLEND_OP_ADD;
	upsampleBlend.alpha.src = VK_BLEND_FACTOR_ONE;
	upsampleBlend.alpha.dst = VK_BLEND_FACTOR_ONE;
	upsampleBlend.writeMask[0] = true;
	upsampleBlend.writeMask[1] = true;
	upsampleBlend.writeMask[2] = true;
	upsampleBlend.writeMask[3] = true;

	m_bloomUpsamplePipeline.setShader(bloomUpsampleShader);
	m_bloomUpsamplePipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_bloomUpsamplePipeline.setDepthTest(false);
	m_bloomUpsamplePipeline.setDepthWrite(false);
	m_bloomUpsamplePipeline.setBlendState(upsampleBlend);

	BoundTexture upsampleImageInput(
		m_bloomTarget->getAttachment(0),
		g_textureManager->getSampler("linear")
	);

	for (int i = 0; i < BLOOM_MIPS; i++)
	{
		m_bloomViews[i] = m_bloomTarget->getAttachment(0)->getView(1, 0, i);
	}

	for (int i = 0; i < BLOOM_MIPS - 1; i++)
	{
		m_bloomUpsampleSets[i] = pool.allocate(bloomUpsampleShader->getDescriptorSetLayout());

		DescriptorWriter()
			.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, upsampleImageInput.getImageInfo(m_bloomViews[i + 1]))
			.updateSet(m_bloomUpsampleSets[i]);
	}
}

void PostProcessPass::render(CommandBuffer &cmd)
{
	renderBloomDownsamples(cmd);

	renderBloomUpsamples(cmd);

	applyHDRTexture(cmd);
}

void PostProcessPass::applyHDRTexture(CommandBuffer &cmd)
{
	struct
	{
		float exposure;
		float bloomIntensity;
	}
	pc;

	pc.exposure = m_exposure;
	pc.bloomIntensity = m_bloomIntensity;

	SubMesh *quadMesh = g_meshLoader->getQuadMesh();

	cmd.beginRecording();
	cmd.beginRendering(g_vkCore->m_swapchain);
	{
		PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_hdrPipeline, cmd.getCurrentRenderInfo());

		cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

		cmd.bindDescriptorSets(0, pipelineData.layout, { m_hdrSet }, {});

		cmd.pushConstants(
			pipelineData.layout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(pc),
			&pc
		);

		quadMesh->render(cmd);
	}
	cmd.endRendering();
	cmd.submit();
}

void PostProcessPass::renderBloomDownsamples(CommandBuffer &cmd)
{
	struct
	{
		int mipLevel;
	}
	pc;

	Texture *attachment = m_bloomTarget->getAttachment(0);
	SubMesh *quad = g_meshLoader->getQuadMesh();

	cmd.beginRecording();

	for (int mipLevel = 0; mipLevel < attachment->getMipLevels(); mipLevel++)
	{
		int width  = attachment->getWidth()  >> mipLevel;
		int height = attachment->getHeight() >> mipLevel;

		RenderInfo info;
		info.setSize(width, height);
		info.addColourAttachment(
			VK_ATTACHMENT_LOAD_OP_LOAD,
			m_bloomViews[mipLevel],
			attachment->getFormat()
		);

		cmd.beginRendering(info);
		{
			PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_bloomDownsamplePipeline, cmd.getCurrentRenderInfo());

			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			cmd.bindDescriptorSets(0, pipelineData.layout, { m_bloomDownsampleSet }, {});

			pc.mipLevel = mipLevel;

			cmd.pushConstants(
				pipelineData.layout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(pc),
				&pc
			);

			cmd.setViewport({ 0.0f, 0.0f, (float)width, (float)height });

			quad->render(cmd);
		}
		cmd.endRendering();
	}

	cmd.submit();
}

void PostProcessPass::renderBloomUpsamples(CommandBuffer &cmd)
{
	struct
	{
		float filterRadius;
	}
	pc;

	pc.filterRadius = m_bloomRadius;

	Texture *attachment = m_bloomTarget->getAttachment(0);
	SubMesh *quad = g_meshLoader->getQuadMesh();

	for (int mipLevel = attachment->getMipLevels() - 1; mipLevel > 0; mipLevel--)
	{
		int dstWidth  = attachment->getWidth()  >> (mipLevel - 1);
		int dstHeight = attachment->getHeight() >> (mipLevel - 1);

		RenderInfo info;
		info.setSize(dstWidth, dstHeight);
		info.addColourAttachment(
			VK_ATTACHMENT_LOAD_OP_LOAD,
			m_bloomViews[mipLevel - 1],
			attachment->getFormat()
		);

		cmd.beginRecording();
		cmd.beginRendering(info);
		{
			PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_bloomUpsamplePipeline, info);

			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			cmd.bindDescriptorSets(0, pipelineData.layout, { m_bloomUpsampleSets[mipLevel - 1] }, {});

			cmd.setViewport({ 0.0f, 0.0f, (float)dstWidth, (float)dstHeight });

			cmd.pushConstants(
				pipelineData.layout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(pc),
				&pc
			);

			quad->render(cmd);
		}
		cmd.endRendering();
		cmd.submit();
	}
}

float PostProcessPass::getExposure() const
{
	return m_exposure;
}

void PostProcessPass::setExposure(float exposure)
{
	m_exposure = exposure;
}

float PostProcessPass::getBloomRadius() const
{
	return m_bloomRadius;
}

void PostProcessPass::setBloomRadius(float radius)
{
	m_bloomRadius = radius;
}

float PostProcessPass::getBloomIntensity() const
{
	return m_bloomIntensity;
}

void PostProcessPass::setBloomIntensity(float intensity)
{
	m_bloomIntensity = intensity;
}
