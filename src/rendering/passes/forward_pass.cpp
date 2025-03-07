#include "forward_pass.h"

#include "vulkan/command_buffer.h"

#include "../camera.h"
#include "../material_system.h"
#include "../mesh.h"
#include "../sub_mesh.h"
#include "../render_object.h"

using namespace llt;

void ForwardPass::init(DescriptorPoolDynamic& pool)
{
}

void ForwardPass::cleanUp()
{
}

void ForwardPass::render(CommandBuffer &buffer, const Camera &camera, const Vector<SubMesh *> &renderList)
{
	if (renderList.size() <= 0)
		return;

	auto *globalBuffer = g_materialSystem->getGlobalBuffer();
	auto *instanceBuffer = g_materialSystem->getInstanceBuffer();

	globalBuffer->getParameters().setValue<glm::mat4>("projMatrix", camera.getProj());
	globalBuffer->getParameters().setValue<glm::mat4>("viewMatrix", camera.getView());
	globalBuffer->getParameters().setValue<glm::vec4>("viewPos", { camera.position.x, camera.position.y, camera.position.z, 0.0f });
	globalBuffer->pushParameters();

	uint64_t currentMaterialHash = 0;

	for (int i = 0; i < renderList.size(); i++)
	{
		SubMesh *mesh = renderList[i];
		Material *mat = mesh->getMaterial();

		if (i == 0 || currentMaterialHash != mat->getHash())
		{
			mat->bindPipeline(buffer, SHADER_PASS_FORWARD);

			buffer.setShader(mat->m_passes[SHADER_PASS_FORWARD].shader);

			currentMaterialHash = mat->getHash();
		}

		glm::mat4 transform = mesh->getParent()->getOwner()->transform.getMatrix();

		instanceBuffer->getParameters().setValue<glm::mat4>("modelMatrix", transform);
		instanceBuffer->getParameters().setValue<glm::mat4>("normalMatrix", glm::transpose(glm::inverse(transform)));
		instanceBuffer->pushParameters();

		mat->bindDescriptorSets(buffer, SHADER_PASS_FORWARD);

		mesh->render(buffer);
	}
}
