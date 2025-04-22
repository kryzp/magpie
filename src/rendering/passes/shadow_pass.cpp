#include "shadow_pass.h"

#include "core/app.h"

#include "math/calc.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"
#include "vulkan/vertex_format.h"
#include "vulkan/gpu_buffer.h"

#include "../scene.h"
#include "../light.h"
#include "../model.h"

using namespace mgp;

void ShadowMapAtlas::init(VulkanCore *core)
{
	m_atlas = new Image(
		core,
		ATLAS_SIZE, ATLAS_SIZE, 1,
		core->getDepthFormat(),
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_atlas->allocate();
}

void ShadowMapAtlas::destroy()
{
	delete m_atlas;
}

bool ShadowMapAtlas::allocate(ShadowMapAtlas::AtlasRegion* region, unsigned quality)
{
	unsigned width = m_atlas->getWidth() >> quality;
	unsigned height = m_atlas->getHeight() >> quality;

	for (int i = 0; i < quality + 1; i++)
	{
		for (int j = 0; j < quality + 1; j++)
		{
			AtlasRegion potential;
			potential.area.x = j * width;
			potential.area.y = i * height;
			potential.area.w = width;
			potential.area.h = height;

			bool intersection = false;

			for (auto &r : m_regions)
			{
				if (r.area.intersects(potential.area))
				{
					intersection = true;
					break;
				}
			}

			if (!intersection)
			{
				m_regions.push_back(potential);
				(*region) = potential;

				return true;
			}
		}
	}

	return false;
}

bool ShadowMapAtlas::adaptiveAlloc(AtlasRegion *region, unsigned idealQuality, int worstQuality)
{
	if (idealQuality >= worstQuality)
		return false;

	if (!allocate(region, idealQuality))
		return adaptiveAlloc(region, idealQuality + 1, worstQuality);

	return true;
}

RenderGraph::AttachmentInfo ShadowMapAtlas::getAtlasAttachment() const
{
	RenderGraph::AttachmentInfo attachment;
	attachment.view = m_atlas->getStandardView();
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.clear = { .depthStencil = { 1.0f, 0 } };

	return attachment;
}

void ShadowMapAtlas::clear()
{
	m_regions.clear();
}

VulkanCore *ShadowPass::m_core = nullptr;
App *ShadowPass::m_app = nullptr;
GPUBuffer *ShadowPass::m_lightsBuffer = nullptr;

void ShadowPass::init(VulkanCore *core, App *app)
{
	m_core = core;
	m_app = app;

	m_lightsBuffer = new GPUBuffer(
		m_core,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(GPUPointLight) * MAX_POINT_LIGHTS
	);
}

void ShadowPass::destroy()
{
	delete m_lightsBuffer;
}

void ShadowPass::renderShadows(Scene& scene, ShadowMapAtlas &atlas)
{
	GraphicsPipelineDef shadowMapPipeline;
	shadowMapPipeline.setShader(m_app->getShader("shadow_map"));
	shadowMapPipeline.setVertexFormat(&vtx::MODEL_VERTEX_FORMAT);

	// this is bad but im doing it to get stuff running
	// ideally shadow maps would be drawn once and only updated when they
	// actually have to change
	// also they shouldn't allocate a new space every time
	atlas.clear();

	m_core->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setOutputAttachments({ atlas.getAtlasAttachment() })
		.setBuildFn([&, shadowMapPipeline](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
			PipelineData pipelineData = m_core->getPipelineCache().fetchGraphicsPipeline(shadowMapPipeline, info);

			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			for (int i = 0; i < scene.getPointLightCount(); i++)
			{
				auto &light = scene.getPointLights()[i];

				glm::vec3 pos = light.getPosition();
				glm::vec3 col = light.getColour().getDisplayColour();
				glm::vec3 dir = light.getDirection();

				GPUPointLight gpuLight = {};
				gpuLight.position = { pos.x, pos.y, pos.z, 0.0f };
				gpuLight.colour = { col.x * light.getIntensity(), col.y * light.getIntensity(), col.z * light.getIntensity(), 0.0f };

				if (light.isShadowCaster())
				{
					ShadowMapAtlas::AtlasRegion region;

					if (atlas.adaptiveAlloc(&region, 3, 6))
					{
						light.setShadowAtlasRegion(region);

						Camera lightCamera((float)region.area.w / (float)region.area.h, 75.0f, 0.01f, 25.0f);
						lightCamera.position = pos;
						lightCamera.direction = dir;

						struct
						{
							glm::mat4 proj;
							glm::mat4 view;
						}
						pc;

						pc.proj = lightCamera.getProj();
						pc.view = lightCamera.getView();

						cmd.pushConstants(
							pipelineData.layout,
							VK_SHADER_STAGE_ALL_GRAPHICS,
							sizeof(pc),
							&pc
						);

						cmd.setViewport({ (float)region.area.x, (float)region.area.y, (float)region.area.w, (float)region.area.h });
						cmd.setScissor({ { (int)region.area.x, (int)region.area.y }, { region.area.w, region.area.h } });

						scene.foreachMesh([&](uint32_t meshIndex, Mesh *mesh) -> bool
						{
							mesh->bind(cmd);
							cmd.drawIndexed(mesh->getIndexCount());

							return true; // continue
						});

						float regionX = (float)region.area.x / (float)ShadowMapAtlas::ATLAS_SIZE;
						float regionY = (float)region.area.y / (float)ShadowMapAtlas::ATLAS_SIZE;
						float regionW = (float)region.area.w / (float)ShadowMapAtlas::ATLAS_SIZE;
						float regionH = (float)region.area.h / (float)ShadowMapAtlas::ATLAS_SIZE;

						gpuLight.atlasRegion = { regionX, regionY, regionW, regionH };

						gpuLight.lightSpaceMatrix = lightCamera.getProj() * lightCamera.getView();
					}
					else
					{
						MGP_ERROR("couldn't allocate enough room on shadowmap for light");
					}
				}

				m_lightsBuffer->write(
					&gpuLight,
					sizeof(GPUPointLight),
					sizeof(GPUPointLight) * i
				);
			}
		}));
}
