#include "shadow_pass.h"

#include "core/app.h"

#include "math/calc.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"
#include "vulkan/vertex_format.h"

#include "../scene.h"
#include "../light.h"
#include "../model.h"

using namespace mgp;

void ShadowMapManager::init(VulkanCore *core)
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

void ShadowMapManager::destroy()
{
	delete m_atlas;
}

bool ShadowMapManager::allocate(ShadowMapManager::AtlasRegion* region, unsigned quality)
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

bool ShadowMapManager::adaptiveAlloc(AtlasRegion *region, unsigned idealQuality, int worstQuality)
{
	if (idealQuality >= worstQuality)
		return false;

	if (!allocate(region, idealQuality))
		return adaptiveAlloc(region, idealQuality + 1, worstQuality);

	return true;
}

void ShadowMapManager::clear()
{
	m_regions.clear();
}

Image *ShadowMapManager::getAtlas()
{
	return m_atlas;
}

const Image *ShadowMapManager::getAtlas() const
{
	return m_atlas;
}

VulkanCore *ShadowPass::m_core = nullptr;
App *ShadowPass::m_app = nullptr;
ShadowMapManager ShadowPass::m_shadowMaps;

void ShadowPass::init(VulkanCore *core, App *app)
{
	m_core = core;
	m_app = app;

	m_shadowMaps.init(core);
}

void ShadowPass::destroy()
{
	m_shadowMaps.destroy();
}

void ShadowPass::renderShadows(Scene& scene)
{
	GraphicsPipelineDefinition shadowMapPipeline;
	shadowMapPipeline.setShader(m_app->getShader("shadow_map"));
	shadowMapPipeline.setVertexFormat(&vtx::MODEL_VERTEX_FORMAT);

	RenderGraph::AttachmentInfo atlasAttachment;
	atlasAttachment.view = m_shadowMaps.getAtlas()->getStandardView();
	atlasAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	atlasAttachment.clear = { .depthStencil = { 1.0f, 0 } };

	// this is bad but im doing it to get stuff running
	// ideally shadow maps would be drawn once and only updated when they
	// actually have to change
	// also they shouldn't allocate a new space every time
	m_shadowMaps.clear();

	m_core->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setDepthStencilAttachment(atlasAttachment)
		.setBuildFn([&, shadowMapPipeline](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
			PipelineData pipelineData = m_core->getPipelineCache().fetchGraphicsPipeline(shadowMapPipeline, info);

			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			for (int i = 0; i < scene.getPointLightCount(); i++)
			{
				auto &light = scene.getPointLights()[i];

				if (light.isShadowCaster())
				{
					ShadowMapManager::AtlasRegion region;

					if (m_shadowMaps.adaptiveAlloc(&region, 3, 6))
					{
						light.setShadowAtlasRegion(region);

						Camera lightCamera((float)region.area.w / (float)region.area.h, 75.0f, 0.01f, 25.0f);
						lightCamera.position = light.getPosition();
						lightCamera.direction = light.getDirection();

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
					}
					else
					{
						MGP_ERROR("couldn't allocate enough room on shadowmap for light");
					}
				}

				// todo: move deferred light stuff here
			}
		}));
}

ShadowMapManager &ShadowPass::getShadowMapManager()
{
	return m_shadowMaps;
}
