#include "post_process_pass.h"

#include "../render_target_mgr.h"
#include "../texture_mgr.h"
#include "../shader_mgr.h"
#include "../mesh_loader.h"

#include "math/colour.h"

#include "vulkan/backend.h"
#include "vulkan/shader.h"
#include "vulkan/pipeline.h"
#include "vulkan/descriptor_builder.h"
#include "vulkan/descriptor_allocator.h"

llt::PostProcessPass llt::g_postProcessPass;

using namespace llt;

void PostProcessPass::init(DescriptorPoolDynamic &pool)
{
	initDefaultValues();

	createBloomResources(pool);
	createHDRResources(pool);
}

void PostProcessPass::cleanUp()
{
}

void PostProcessPass::initDefaultValues()
{
	m_exposure = 2.0f;

	m_bloomRadius = 0.0f;
	m_bloomIntensity = 0.0f;
}

void PostProcessPass::createHDRResources(DescriptorPoolDynamic &pool)
{
	BoundTexture sceneTexture(
		g_renderTargetManager->get("target")->getAttachment(0),
		g_textureManager->getSampler("linear")
	);

	BoundTexture bloomTexture(
		m_bloomTarget->getAttachment(0),
		g_textureManager->getSampler("linear")
	);

	ShaderEffect *hdrTonemappingShader = g_shaderManager->getEffect("hdrTonemapping");

	m_hdrSet = pool.allocate(hdrTonemappingShader->getDescriptorSetLayout());

	DescriptorWriter hdrWriter;
	hdrWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sceneTexture.getStandardImageInfo());
	hdrWriter.writeImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, bloomTexture.getStandardImageInfo());
	hdrWriter.updateSet(m_hdrSet);

	m_hdrPipeline.setShader(hdrTonemappingShader);
	m_hdrPipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_hdrPipeline.setDepthTest(false);
	m_hdrPipeline.setDepthWrite(false);
}

void PostProcessPass::createBloomResources(DescriptorPoolDynamic &pool)
{
	m_bloomTarget = g_renderTargetManager->createTarget(
		"bloomTarget",
		g_vulkanBackend->m_backbuffer->getWidth(),
		g_vulkanBackend->m_backbuffer->getHeight(),
		{
			VK_FORMAT_R32G32B32A32_SFLOAT
		},
		VK_SAMPLE_COUNT_1_BIT,
		5
	);

	m_bloomTarget->createDepthAttachment();
	m_bloomTarget->toggleClear(false);
	m_bloomTarget->setClearColours(Colour::black());

	// ---

	ShaderEffect *bloomDownsampleShader = g_shaderManager->getEffect("bloomDownsample");

	m_bloomDownsampleSet = pool.allocate(bloomDownsampleShader->getDescriptorSetLayout());

	m_bloomDownsamplePipeline.setShader(bloomDownsampleShader);
	m_bloomDownsamplePipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_bloomDownsamplePipeline.setDepthTest(false);
	m_bloomDownsamplePipeline.setDepthWrite(false);

	// ---

	ShaderEffect *bloomUpsampleShader = g_shaderManager->getEffect("bloomUpsample");

	m_bloomUpsampleSet = pool.allocate(bloomUpsampleShader->getDescriptorSetLayout());

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
}

void PostProcessPass::render(CommandBuffer &cmd)
{
	renderBloomDownsamples(cmd);
	renderBloomUpsamples(cmd);

	applyHDRTexture(cmd);
}

void PostProcessPass::applyHDRTexture(CommandBuffer &cmd)
{
	SubMesh *quadMesh = g_meshLoader->getQuadMesh();

	cmd.beginRendering(g_vulkanBackend->m_backbuffer);
	{
		PipelineData pipelineData = g_vulkanBackend->getPipelineCache().fetchGraphicsPipeline(m_hdrPipeline, cmd.getCurrentRenderInfo());

		cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

		cmd.bindDescriptorSets(0, pipelineData.layout, { m_hdrSet }, {});

		struct
		{
			float exposure;
			float bloomIntensity;
		}
		pc;

		pc.exposure = m_exposure;
		pc.bloomIntensity = m_bloomIntensity;

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
	Texture *attachment = m_bloomTarget->getAttachment(0);
	SubMesh *quad = g_meshLoader->getQuadMesh();

	for (int mipLevel = 0; mipLevel < attachment->getMipLevels(); mipLevel++)
	{
		int width = attachment->getWidth() >> mipLevel;
		int height = attachment->getHeight() >> mipLevel;

		VkImageView view = attachment->createView(1, 0, mipLevel);

		RenderInfo info;
		info.setSize(width, height);
		info.addColourAttachment(
			VK_ATTACHMENT_LOAD_OP_LOAD,
			view,
			attachment->getFormat()
		);

		cmd.beginRendering(info);
		{
			PipelineData pipelineData = g_vulkanBackend->getPipelineCache().fetchGraphicsPipeline(m_bloomDownsamplePipeline, cmd.getCurrentRenderInfo());

			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			BoundTexture downsampleInputImage(
				g_renderTargetManager->get("target")->getAttachment(0),
				g_textureManager->getSampler("linear")
			);

			DescriptorWriter downsampleWriter;
			downsampleWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, downsampleInputImage.getStandardImageInfo());
			downsampleWriter.updateSet(m_bloomDownsampleSet);

			cmd.bindDescriptorSets(0, pipelineData.layout, { m_bloomDownsampleSet }, {});

			struct
			{
				int mipLevel;
			}
			pc;

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
		cmd.submit();

		vkWaitForFences(g_vulkanBackend->m_device, 1, &g_vulkanBackend->m_graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);
		
		vkDestroyImageView(g_vulkanBackend->m_device, view, nullptr);
	}
}

void PostProcessPass::renderBloomUpsamples(CommandBuffer &cmd)
{
	Texture *attachment = m_bloomTarget->getAttachment(0);
	SubMesh *quad = g_meshLoader->getQuadMesh();

	for (int mipLevel = attachment->getMipLevels() - 1; mipLevel > 0; mipLevel--)
	{
		int dstWidth  = attachment->getWidth()  >> (mipLevel - 1);
		int dstHeight = attachment->getHeight() >> (mipLevel - 1);

		VkImageView srcView = m_bloomTarget->getAttachment(0)->createView(1, 0, mipLevel);
		VkImageView dstView = attachment->createView(1, 0, mipLevel - 1);

		RenderInfo info;
		info.setSize(dstWidth, dstHeight);
		info.addColourAttachment(
			VK_ATTACHMENT_LOAD_OP_LOAD,
			dstView,
			attachment->getFormat()
		);

		cmd.beginRendering(info);
		{
			PipelineData pipelineData = g_vulkanBackend->getPipelineCache().fetchGraphicsPipeline(m_bloomUpsamplePipeline, info);

			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			BoundTexture upsampleImageInput(
				m_bloomTarget->getAttachment(0),
				g_textureManager->getSampler("linear")
			);

			DescriptorWriter upsampleWriter;
			upsampleWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, upsampleImageInput.getImageInfo(srcView));
			upsampleWriter.updateSet(m_bloomUpsampleSet);

			cmd.bindDescriptorSets(0, pipelineData.layout, { m_bloomUpsampleSet }, {});

			cmd.setViewport({ 0.0f, 0.0f, (float)dstWidth, (float)dstHeight });

			struct
			{
				float filterRadius;
			}
			pc;

			pc.filterRadius = m_bloomRadius;

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

		vkWaitForFences(g_vulkanBackend->m_device, 1, &g_vulkanBackend->m_graphicsQueue.getCurrentFrame().inFlightFence, VK_TRUE, UINT64_MAX);

		vkDestroyImageView(g_vulkanBackend->m_device, srcView, nullptr);
		vkDestroyImageView(g_vulkanBackend->m_device, dstView, nullptr);
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
