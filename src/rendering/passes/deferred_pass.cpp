#include "deferred_pass.h"

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

using namespace mgp;

// todo: this should use instance id instead
struct BindlessPushConstants
{
	bindless::Handle frameDataBuffer_ID;
	bindless::Handle transformBuffer_ID;
	bindless::Handle materialDataBuffer_ID;
	bindless::Handle lightBuffer_ID;

	bindless::Handle transform_ID;

	bindless::Handle irradianceMap_ID;
	bindless::Handle prefilterMap_ID;
	bindless::Handle brdfLUT_ID;

	bindless::Handle material_ID;

	bindless::Handle cubemapSampler_ID;
	bindless::Handle textureSampler_ID;
	bindless::Handle shadowAtlasSampler_ID;

	bindless::Handle shadowAtlas_ID;
};

VulkanCore *DeferredPass::m_core = nullptr;
App *DeferredPass::m_app = nullptr;
Image *DeferredPass::m_brdfLUT = nullptr;
GPUBuffer *DeferredPass::m_lightsBuffer = nullptr;

void DeferredPass::init(VulkanCore *core, App *app)
{
	m_core = core;
	m_app = app;

	m_lightsBuffer = new GPUBuffer(
		m_core,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(GPUPointLight) * MAX_POINT_LIGHTS
	);

	precomputeBRDF();
}

void DeferredPass::destroy()
{
	delete m_lightsBuffer;
	delete m_brdfLUT;
}

void DeferredPass::render(CommandBuffer &cmd, const RenderInfo &info, const Camera& camera, Scene& scene, const ImageView *shadowAtlas)
{
	populateLightsBuffer(scene);

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
			.lightBuffer_ID				= m_lightsBuffer->getBindlessHandle(),

			.transform_ID				= 0,

			.irradianceMap_ID			= m_app->m_environmentProbe.irradiance->getStandardView()->getBindlessHandle(),
			.prefilterMap_ID			= m_app->m_environmentProbe.prefilter->getStandardView()->getBindlessHandle(),

			.brdfLUT_ID					= m_brdfLUT->getStandardView()->getBindlessHandle(),

			.material_ID				= mat->bindlessHandle,

			.cubemapSampler_ID			= m_app->m_linearSampler->getBindlessHandle(),
			.textureSampler_ID			= m_app->m_linearSampler->getBindlessHandle(),
			.shadowAtlasSampler_ID		= m_app->m_nearestSampler->getBindlessHandle(),

			.shadowAtlas_ID				= shadowAtlas->getBindlessHandle()
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

void DeferredPass::populateLightsBuffer(const Scene& scene)
{
	auto &lights = scene.getPointLights();

	for (int i = 0; i < scene.getPointLightCount(); i++)
	{
		auto &light = lights[i];

		glm::vec3 pos = light.getPosition();
		glm::vec3 col = light.getColour().getPremultiplied().getDisplayColour();

		auto &region = light.getShadowAtlasRegion();

		Camera lightCamera(1.0f, 75.0f, 0.01f, 25.0f);
		lightCamera.position = light.getPosition();
		lightCamera.direction = light.getDirection();

		GPUPointLight gpuLight = {};
		gpuLight.position = { pos.x, pos.y, pos.z, 0.0f };
		gpuLight.colour = { col.x * light.getIntensity(), col.y * light.getIntensity(), col.z * light.getIntensity(), 0.0f };

		if (light.isShadowCaster())
		{
			float regionX = (float)region.area.x / (float)ShadowMapManager::ATLAS_SIZE;
			float regionY = (float)region.area.y / (float)ShadowMapManager::ATLAS_SIZE;
			float regionW = (float)region.area.w / (float)ShadowMapManager::ATLAS_SIZE;
			float regionH = (float)region.area.h / (float)ShadowMapManager::ATLAS_SIZE;

			gpuLight.atlasRegion = { regionX, regionY, regionW, regionH };

			gpuLight.lightSpaceMatrix = lightCamera.getProj() * lightCamera.getView();
		}

		m_lightsBuffer->write(
			&gpuLight,
			sizeof(GPUPointLight),
			sizeof(GPUPointLight) * i
		);
	}
}

void DeferredPass::precomputeBRDF()
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

		GraphicsPipelineDefinition brdfIntegrationPipeline;
		brdfIntegrationPipeline.setShader(brdfLUTShader);
		brdfIntegrationPipeline.setDepthTest(false);
		brdfIntegrationPipeline.setDepthWrite(false);

		PipelineData pipelineData = m_core->getPipelineCache().fetchGraphicsPipeline(brdfIntegrationPipeline, info);

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		cmd.beginRendering(info);
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);
			cmd.draw(3);
		}
		cmd.endRendering();

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	instantSubmit.submit();
}
