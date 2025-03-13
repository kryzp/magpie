#include "forward_pass.h"

#include "vulkan/backend.h"
#include "vulkan/command_buffer.h"

#include "../camera.h"
#include "../material_system.h"
#include "../mesh.h"
#include "../render_object.h"

llt::ForwardPass llt::g_forwardPass;

using namespace llt;

void ForwardPass::init()
{
}

void ForwardPass::cleanUp()
{
}

void ForwardPass::render(CommandBuffer &cmd, const Camera &camera, const Vector<SubMesh *> &renderList)
{
	if (renderList.size() <= 0)
		return;

	g_materialSystem->m_globalData.proj = camera.getProj();
	g_materialSystem->m_globalData.view = camera.getView();
	g_materialSystem->m_globalData.cameraPosition = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
	g_materialSystem->pushGlobalData();

	uint64_t currentMaterialHash = 0;

	for (int i = 0; i < renderList.size(); i++)
	{
		SubMesh *mesh = renderList[i];
		Material *mat = mesh->getMaterial();

		PipelineData data = g_vulkanBackend->getPipelineCache().fetchGraphicsPipeline(mat->getPipelineDef(SHADER_PASS_FORWARD), cmd.getCurrentRenderInfo());

		if (i == 0 || currentMaterialHash != mat->getHash())
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, data.pipeline);

			currentMaterialHash = mat->getHash();
		}

		glm::mat4 transform = mesh->getParent()->getOwner()->transform.getMatrix();

		g_materialSystem->m_instanceData.model = transform;
		g_materialSystem->m_instanceData.normalMatrix = glm::transpose(glm::inverse(transform));
		g_materialSystem->pushInstanceData();

		mat->bindDescriptorSets(cmd, SHADER_PASS_FORWARD, data.layout);

		mesh->render(cmd);
	}
}
