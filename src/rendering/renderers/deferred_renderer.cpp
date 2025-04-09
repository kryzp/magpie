#include "deferred_renderer.h"

#include "core/app.h"
#include "core/common.h"

#include "../scene.h"
#include "../material.h"
#include "../model.h"

#include "vulkan/core.h"
#include "vulkan/sync.h"
#include "vulkan/image.h"
#include "vulkan/render_info.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/sampler.h"
#include "vulkan/shader.h"

// todo: this should use instance id instead
struct BindlessPushConstants
{
	int frameDataBuffer_ID;
	int transformBuffer_ID;
	int materialDataBuffer_ID;

	int transform_ID;

	int irradianceMap_ID;
	int prefilterMap_ID;
	int brdfLUT_ID;

	int material_ID;

	int cubemapSampler_ID;
	int textureSampler_ID;
};

using namespace mgp;

VulkanCore *DeferredRenderer::m_core = nullptr;
App *DeferredRenderer::m_app = nullptr;

Image *DeferredRenderer::m_brdfLUT = nullptr;

void DeferredRenderer::init(VulkanCore *core, App *app)
{
	m_core = core;
	m_app = app;

	precomputeBRDF();
}

void DeferredRenderer::destroy()
{
	delete m_brdfLUT;
}

void DeferredRenderer::render(CommandBuffer &cmd, const RenderInfo &info, const Camera& camera, Scene& scene)
{
	uint64_t currentPipelineHash = 0;

	scene.foreachMesh([&](uint32_t meshIndex, Mesh *mesh) -> bool
	{
		Material *mat = mesh->getMaterial();

		PipelineData pipelineData = m_core->getPipelineCache().fetchGraphicsPipeline(mat->passes[SHADER_PASS_FORWARD], info);

		if (meshIndex == 0)
		{
			cmd.bindDescriptorSets(
				0,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineData.layout,
				{ m_core->getBindlessResources().getSet() },
				{}
			);
		}

		if (meshIndex == 0 || currentPipelineHash != mat->getHash())
		{
			cmd.bindPipeline(
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineData.pipeline
			);

			currentPipelineHash = mat->getHash();
		}

		BindlessPushConstants pc = {
			.frameDataBuffer_ID			= m_app->m_frameConstantsBuffer->getBindlessHandle(),
			.transformBuffer_ID			= m_app->m_transformDataBuffer->getBindlessHandle(),
			.materialDataBuffer_ID		= m_app->m_bindlessMaterialTable->getBindlessHandle(),

			.transform_ID				= 0,

			.irradianceMap_ID			= m_app->m_environmentProbe.irradiance->getStandardView()->getBindlessHandle(),
			.prefilterMap_ID			= m_app->m_environmentProbe.prefilter->getStandardView()->getBindlessHandle(),

			.brdfLUT_ID					= m_brdfLUT->getStandardView()->getBindlessHandle(),

			.material_ID				= mat->bindlessHandle,

			.cubemapSampler_ID			= m_app->m_linearSampler->getBindlessHandle(),
			.textureSampler_ID			= m_app->m_linearSampler->getBindlessHandle()
		};

		cmd.pushConstants(
			pipelineData.layout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(BindlessPushConstants),
			&pc
		);

		mesh->bind(cmd);

		cmd.drawIndexed(mesh->getIndexCount());

		return true; // continue
	});
}

void DeferredRenderer::precomputeBRDF()
{
	InstantSubmitSync instantSubmit(m_core);
	CommandBuffer &cmd = instantSubmit.begin();
	{
		MGP_LOG("Precomputing BRDF...");

		const int BRDF_RESOLUTION = 512;

		m_brdfLUT = new Image(
			m_core,
			BRDF_RESOLUTION, BRDF_RESOLUTION, 1,
			VK_FORMAT_R32G32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_TILING_OPTIMAL,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		m_brdfLUT->allocate();

		RenderInfo info(m_core);
		info.setSize(BRDF_RESOLUTION, BRDF_RESOLUTION);
		info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *m_brdfLUT->getStandardView(), nullptr);

		Shader *brdfLUTShader = m_app->getShader("brdf_lut");

		VkDescriptorSet set = m_app->allocateSet(brdfLUTShader->getDescriptorSetLayouts());

		GraphicsPipelineDefinition brdfIntegrationPipeline;
		brdfIntegrationPipeline.setShader(brdfLUTShader);
		brdfIntegrationPipeline.setDepthTest(false);
		brdfIntegrationPipeline.setDepthWrite(false);

		PipelineData pipelineData = m_core->getPipelineCache().fetchGraphicsPipeline(brdfIntegrationPipeline, info);

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		cmd.beginRendering(info);
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			cmd.bindDescriptorSets(0, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, { set }, {});

			cmd.setViewport({ 0, 0, BRDF_RESOLUTION, BRDF_RESOLUTION });

			cmd.draw(3);
		}
		cmd.endRendering();

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	instantSubmit.submit();
}
