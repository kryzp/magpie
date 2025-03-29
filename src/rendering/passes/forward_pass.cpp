#include "forward_pass.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"

#include "../gpu_buffer_mgr.h"
#include "../texture_mgr.h"
#include "../camera.h"
#include "../material_system.h"
#include "../mesh.h"
#include "../render_object.h"

llt::ForwardPass llt::g_forwardPass;

using namespace llt;

void ForwardPass::init()
{
	m_frameConstantsBuffer = g_gpuBufferManager->createStorageBuffer(sizeof(FrameConstants));
	m_transformationBuffer = g_gpuBufferManager->createStorageBuffer(sizeof(TransformData) * 16);

	FrameConstants frameConstants = {
		.proj = glm::identity<glm::mat4>(),
		.view = glm::identity<glm::mat4>(),
		.cameraPosition = glm::zero<glm::vec4>()
	};

	m_frameConstantsBuffer->writeDataToMe(&frameConstants, sizeof(FrameConstants), 0);

	// temp max 16 transformed entities :))))))
	TransformData transformData[16] = {};

	for (int i = 0; i < 16; i++)
	{
		transformData[i] = {
			.model = glm::identity<glm::mat4>(),
			.normalMatrix = glm::identity<glm::mat4>()
		};
	}

	m_transformationBuffer->writeDataToMe(&transformData, sizeof(TransformData) * 16, 0);
}

void ForwardPass::dispose()
{
	delete m_frameConstantsBuffer;
	delete m_transformationBuffer;
}

void ForwardPass::render(CommandBuffer &cmd, const Camera &camera, const Vector<SubMesh *> &renderList)
{
	if (renderList.size() <= 0)
		return;

	FrameConstants frameConstants = {
		.proj = camera.getProj(),
		.view = camera.getView(),
		.cameraPosition = { camera.position.x, camera.position.y, camera.position.z, 0.0f }
	};

	m_frameConstantsBuffer->writeDataToMe(&frameConstants, sizeof(FrameConstants), 0);

	uint64_t currentMaterialHash = 0;

	for (int i = 0; i < renderList.size(); i++)
	{
		SubMesh *mesh = renderList[i];
		Material *mat = mesh->getMaterial();

		PipelineData data = g_vkCore->getPipelineCache().fetchGraphicsPipeline(mat->getPipelineDef(SHADER_PASS_FORWARD), cmd.getCurrentRenderInfo());

		if (i == 0 || currentMaterialHash != mat->getHash())
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipeline);

			cmd.bindDescriptorSets(
				0,
				data.layout,
				{ g_bindlessResources->getSet() },
				{}
			);

			currentMaterialHash = mat->getHash();
		}

		static float t = 0.f;
		t += 0.01f;

		glm::mat4 transformMatrix =
			glm::rotate(glm::identity<glm::mat4>(), t, {0.0f, 1.0f, 0.0}) *
			mesh->getParent()->getOwner()->transform.getMatrix();

		TransformData transformData = {
			.model = transformMatrix,
			.normalMatrix = glm::transpose(glm::inverse(transformMatrix))
		};

		m_transformationBuffer->writeDataToMe(&transformData, sizeof(TransformData), sizeof(TransformData) * i);

		struct
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
		}
		pushConstants;
		{
			pushConstants.frameDataBuffer_ID	= m_frameConstantsBuffer->getBindlessHandle().id;
			pushConstants.transformBuffer_ID	= m_transformationBuffer->getBindlessHandle().id;
			pushConstants.materialDataBuffer_ID = g_materialSystem->getRegistry().getMaterialIdBuffer()->getBindlessHandle().id;

			pushConstants.transform_ID			= 0;

			pushConstants.irradianceMap_ID		= g_materialSystem->getIrradianceMap()->getStandardView().getBindlessHandle().id;
			pushConstants.prefilterMap_ID		= g_materialSystem->getPrefilterMap()->getStandardView().getBindlessHandle().id;
			pushConstants.brdfLUT_ID			= g_materialSystem->getBRDFLUT()->getStandardView().getBindlessHandle().id;

			pushConstants.material_ID			= mat->getBindlessHandle().id;

			pushConstants.cubemapSampler_ID		= g_textureManager->getSampler("linear")->getBindlessHandle().id;
			pushConstants.textureSampler_ID		= g_textureManager->getSampler("linear")->getBindlessHandle().id;
		}

		cmd.pushConstants(
			data.layout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(pushConstants),
			&pushConstants
		);

		mesh->render(cmd);
	}
}
