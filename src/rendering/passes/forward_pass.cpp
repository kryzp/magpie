#include "forward_pass.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"

#include "../texture_mgr.h"
#include "../camera.h"
#include "../material_system.h"
#include "../mesh.h"
#include "../render_object.h"

llt::ForwardPass llt::g_forwardPass;

using namespace llt;

void ForwardPass::init()
{
}

void ForwardPass::dispose()
{
}

void ForwardPass::render(CommandBuffer &cmd, const Camera &camera, const Vector<SubMesh *> &renderList)
{
	if (renderList.size() <= 0)
		return;

	g_bindlessResources->writeFrameConstants({
		.proj = camera.getProj(),
		.view = camera.getView(),
		.cameraPosition = { camera.position.x, camera.position.y, camera.position.z, 0.0f }
	});

	uint64_t currentMaterialHash = 0;

	for (int i = 0; i < renderList.size(); i++)
	{
		SubMesh *mesh = renderList[i];
		Material *mat = mesh->getMaterial();

		PipelineData data = g_vkCore->getPipelineCache().fetchGraphicsPipeline(mat->getPipelineDef(SHADER_PASS_FORWARD), cmd.getCurrentRenderInfo());

		if (i == 0 || currentMaterialHash != mat->getHash())
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipeline);

			currentMaterialHash = mat->getHash();
		}

		static float t = 0.f;
		t += 0.01f;

		glm::mat4 transform = glm::scale(glm::rotate(mesh->getParent()->getOwner()->transform.getMatrix(), t, { 0.0f, 1.0f, 0.0 }), glm::vec3(1.0 + glm::sin(t)*0.5f));

		g_bindlessResources->writeTransformData(i, {
			.model = transform,
			.normalMatrix = glm::transpose(glm::inverse(transform))
		});

		struct
		{
			BindlessResourceHandle transform_ID;

			BindlessResourceHandle irradianceMap_ID;
			BindlessResourceHandle prefilterMap_ID;

			BindlessResourceHandle brdfLUT_ID;

			BindlessResourceHandle diffuseTexture_ID;
			BindlessResourceHandle aoTexture_ID;
			BindlessResourceHandle mrTexture_ID;
			BindlessResourceHandle normalTexture_ID;
			BindlessResourceHandle emissiveTexture_ID;

			BindlessResourceHandle cubemapSampler_ID;
			BindlessResourceHandle textureSampler_ID;
		}
		pushConstants;

		pushConstants.transform_ID = 0;

		pushConstants.irradianceMap_ID = g_materialSystem->getIrradianceMap()->getStandardView().getBindlessHandle();
		pushConstants.prefilterMap_ID = g_materialSystem->getPrefilterMap()->getStandardView().getBindlessHandle();

		pushConstants.brdfLUT_ID = g_materialSystem->getBRDFLUT()->getStandardView().getBindlessHandle();

		pushConstants.diffuseTexture_ID = mat->m_textures[0].getBindlessHandle();
		pushConstants.aoTexture_ID = mat->m_textures[1].getBindlessHandle();
		pushConstants.mrTexture_ID = mat->m_textures[2].getBindlessHandle();
		pushConstants.normalTexture_ID = mat->m_textures[3].getBindlessHandle();
		pushConstants.emissiveTexture_ID = mat->m_textures[4].getBindlessHandle();

		pushConstants.cubemapSampler_ID = g_textureManager->getSampler("linear")->getBindlessHandle();
		pushConstants.textureSampler_ID = g_textureManager->getSampler("linear")->getBindlessHandle();

		cmd.pushConstants(
			data.layout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(pushConstants),
			&pushConstants
		);

		cmd.bindDescriptorSets(
			0,
			data.layout,
			{ g_bindlessResources->getSet() },
			{}
		);

		mesh->render(cmd);
	}
}
