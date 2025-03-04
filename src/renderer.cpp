#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#include "input/input.h"

#include "graphics/backend.h"
#include "graphics/backbuffer.h"
#include "graphics/command_buffer.h"
#include "graphics/material.h"
#include "graphics/material_system.h"
#include "graphics/mesh_loader.h"
#include "graphics/texture.h"
#include "graphics/texture_mgr.h"
#include "graphics/render_target_mgr.h"

#include "math/colour.h"

using namespace llt;

Renderer::Renderer()
	: m_quadMesh()
	, m_skyboxMesh()
	, m_gBuffer(nullptr)
	, m_pushConstants()
//	, m_gpuParticles()
	, m_renderObjects()
	, m_renderList()
	, m_skyboxMaterial()
//	, m_postProcessPipeline()
	, m_blockMesh(nullptr)
{
}

void Renderer::init()
{
	m_pushConstants.setValue<float>("time", 0.0f);

	g_gpuBufferManager->createGlobalStagingBuffers();

	initVertexTypes();

	createQuadMesh();

	g_meshLoader = new MeshLoader();

	g_textureManager->loadDefaultTexturesAndSamplers();
	g_shaderManager->loadDefaultShaders();

	g_materialSystem = new MaterialSystem();
	g_materialSystem->loadDefaultTechniques();
	g_materialSystem->init();

	createSkybox();

	addRenderObjects();

//	m_gBuffer = g_renderTargetManager->createTarget(
//		"gBuffer", // YES I KNOW THIS ISNT A DEFERRED RENDERING GBUFFER BUT WHATEVER
//		m_backbuffer->getWidth(),
//		m_backbuffer->getHeight(),
//		{
//			VK_FORMAT_B8G8R8A8_UNORM, // colour
//			VK_FORMAT_R32G32_SFLOAT, // motion
//			VK_FORMAT_R32G32B32A32_SFLOAT // normals
//		},
//		VK_SAMPLE_COUNT_1_BIT
//	);
//
//	m_gBuffer->createDepthAttachment();
//
//	m_gpuParticles.init(m_shaderParamsBuffer);
//	
//	createPostProcessResources();
}

void Renderer::cleanUp()
{
//	m_descriptorPool.cleanUp();

	m_pushConstants.cleanUp();

	delete g_meshLoader;
	delete g_materialSystem;
}

Vector<RenderObject>::Iterator Renderer::createRenderObject()
{
	return m_renderObjects.emplaceBack();
}

void Renderer::addRenderObjects()
{
//	MaterialData woodMaterialData;
//	woodMaterialData.textures.pushBack({ g_textureManager->getTexture("wood"), g_textureManager->getSampler("linear") });
//	woodMaterialData.technique = "texturedPBR";
//	Material *woodMaterial = g_materialSystem->buildMaterial(woodMaterialData);

//	auto floor = m_renderEntities.emplaceBack();
//	floor->transform.setPosition({ 0.0f, 0.0f, 0.0f });
//	floor->transform.setRotation(0.0f, { 0.0f, 1.0f, 0.0f });
//	floor->transform.setScale({ 10.0f, 1.0f, 10.0f });
//	floor->transform.setOrigin({ 0.0f, 1.0f, 0.0f });
//	floor->mesh = m_blockMesh;
	
	auto assimpModel = createRenderObject();
	assimpModel->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	assimpModel->transform.setRotation(glm::radians(0.0f), { 1.0f, 0.0f, 0.0f });
	assimpModel->transform.setScale({ 4.0f, 4.0f, 4.0f });
	assimpModel->transform.setOrigin({ 0.0f, 0.0f, 0.0f });
	assimpModel->mesh = g_meshLoader->loadMesh("model", "../../res/models/GLTF/DamagedHelmet/DamagedHelmet.gltf");
	assimpModel->mesh->setOwner(&(*assimpModel));
}

void Renderer::createPostProcessResources()
{
/*
	DescriptorLayoutBuilder ppDescriptorLayoutBuilder;
	ppDescriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	BoundTexture gBufferAttachment;
	gBufferAttachment.texture = m_gBuffer->getAttachment(0);
	gBufferAttachment.sampler = g_textureManager->getSampler("linear");

	DescriptorWriter ppDescriptorWriter;
	ppDescriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gBufferAttachment.getImageInfo());
	
	VkDescriptorSetLayout ppDescriptorSetLayout = ppDescriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, nullptr);
	
	ShaderProgram *ppVS = g_shaderManager->create("postProcessVertex", "../../res/shaders/raster/post_process_vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram *ppPS = g_shaderManager->create("postProcessFragment", "../../res/shaders/raster/post_process_ps.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	
	m_postProcessPipeline.setVertexDescriptor(g_modelVertexFormat);
	m_postProcessPipeline.setDepthTest(false);
	m_postProcessPipeline.setDepthWrite(false);
	m_postProcessPipeline.bindShader(ppVS);
	m_postProcessPipeline.bindShader(ppPS);
	m_postProcessPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_postProcessPipeline.setDescriptorSetLayout(ppDescriptorSetLayout);
	
	m_descriptorPool.setSizes(64 * mgc::FRAMES_IN_FLIGHT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	4.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			4.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			0.5f }
	});
	
	m_postProcessDescriptorSet = m_descriptorPool.allocate(ppDescriptorSetLayout);
	
	ppDescriptorWriter.updateSet(m_postProcessDescriptorSet);
*/
}

void Renderer::createQuadMesh()
{
	Vector<ModelVertex> quadVertices = {
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } }
	};

	Vector<uint16_t> quadIndices = {
		0, 1, 2,
		0, 2, 3
	};

	m_quadMesh.build(
		sizeof(ModelVertex),
		quadVertices.data(), quadVertices.size(),
		quadIndices.data(), quadIndices.size()
	);
}

void Renderer::createSkybox()
{
	Vector<ModelVertex> skyboxVertices =
	{
		{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } }
	};

	Vector<uint16_t> skyboxIndices =
	{
		0, 1, 2,
		2, 3, 0,

		5, 4, 7,
		7, 6, 5,

		1, 5, 6,
		6, 2, 1,

		4, 0, 3,
		3, 7, 4,

		4, 5, 1,
		1, 0, 4,

		3, 2, 6,
		6, 7, 3
	};

	m_skyboxMesh.build(
		sizeof(ModelVertex),
		skyboxVertices.data(), skyboxVertices.size(),
		skyboxIndices.data(), skyboxIndices.size()
	);

	MaterialData skyboxData;
	skyboxData.technique = "skybox";
	skyboxData.textures = { { g_textureManager->getTexture("environmentMap"), g_textureManager->getSampler("linear") } };

	m_skyboxMaterial = g_materialSystem->buildMaterial(skyboxData);
}

void Renderer::aggregateSubMeshes(Vector<SubMesh*> &list)
{
	list.clear();

	for (auto &obj : m_renderObjects)
	{
		if (!obj.mesh)
			continue;

		for (int i = 0; i < obj.mesh->getSubmeshCount(); i++)
		{
			list.pushBack(obj.mesh->getSubmesh(i));
		}
	}
}

void Renderer::sortRenderListByMaterialHash(int lo, int hi)
{
	if (lo >= hi || lo < 0) {
		return;
	}

	int pivot = m_renderList[hi]->getMaterial()->getHash();
	int i = lo - 1;

	for (int j = lo; j < hi; j++)
	{
		if (m_renderList[j]->getMaterial()->getHash() <= pivot)
		{
			i++;
			LLT_SWAP(m_renderList[i], m_renderList[j]);
		}
	}

	int partition = i + 1;
	LLT_SWAP(m_renderList[partition], m_renderList[hi]);

	sortRenderListByMaterialHash(lo, partition - 1);
	sortRenderListByMaterialHash(partition + 1, hi);
}

void Renderer::renderObjects(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target)
{
	RenderInfo info = target->getRenderInfo();

	g_materialSystem->getGlobalBuffer()->getParameters().setValue<glm::mat4>("projMatrix", camera.getProj());
	g_materialSystem->getGlobalBuffer()->getParameters().setValue<glm::mat4>("viewMatrix", camera.getView());
	g_materialSystem->getGlobalBuffer()->getParameters().setValue<glm::vec4>("viewPos", { camera.position.x, camera.position.y, camera.position.z, 0.0f });
	g_materialSystem->getGlobalBuffer()->pushParameters();

	aggregateSubMeshes(m_renderList);
	sortRenderListByMaterialHash(0, m_renderList.size() - 1);

	if (m_renderList.size() > 0)
	{
		uint64_t currentMaterialHash = m_renderList[0]->getMaterial()->getHash();
		m_renderList[0]->getMaterial()->bindPipeline(buffer, info, SHADER_PASS_FORWARD);

		for (int i = 0; i < m_renderList.size(); i++)
		{
			SubMesh *mesh = m_renderList[i];
			Material *mat = mesh->getMaterial();

			if (currentMaterialHash != mat->getHash())
			{
				mat->bindPipeline(buffer, info, SHADER_PASS_FORWARD);

				currentMaterialHash = mat->getHash();
			}

			glm::mat4 transform = mesh->getParent()->getOwner()->transform.getMatrix();

			g_materialSystem->getInstanceBuffer()->getParameters().setValue<glm::mat4>("modelMatrix", transform);
			g_materialSystem->getInstanceBuffer()->getParameters().setValue<glm::mat4>("normalMatrix", glm::transpose(glm::inverse(transform)));
			g_materialSystem->getInstanceBuffer()->pushParameters();

			mat->renderMesh(buffer, SHADER_PASS_FORWARD, *mesh);
		}
	}
}

void Renderer::renderSkybox(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target)
{
	g_materialSystem->getGlobalBuffer()->getParameters().setValue<glm::mat4>("projMatrix", camera.getProj());
	g_materialSystem->getGlobalBuffer()->getParameters().setValue<glm::mat4>("viewMatrix", camera.getRotationMatrix());
	g_materialSystem->getGlobalBuffer()->getParameters().setValue<glm::vec4>("viewPos", { camera.position.x, camera.position.y, camera.position.z, 0.0f });
	g_materialSystem->getGlobalBuffer()->pushParameters();

	g_materialSystem->getInstanceBuffer()->getParameters().setValue<glm::mat4>("modelMatrix", glm::identity<glm::mat4>());
	g_materialSystem->getInstanceBuffer()->getParameters().setValue<glm::mat4>("normalMatrix", glm::identity<glm::mat4>());
	g_materialSystem->getInstanceBuffer()->pushParameters();

	RenderInfo info = target->getRenderInfo();

	m_skyboxMaterial->bindPipeline(buffer, info, SHADER_PASS_FORWARD);
	m_skyboxMaterial->renderMesh(buffer, SHADER_PASS_FORWARD, m_skyboxMesh);
}

void Renderer::render(const Camera &camera, float deltaTime)
{
	g_materialSystem->updateImGui();

	ImGui::ShowDemoWindow();
	ImGui::Render();

	for (RenderObject &entity : m_renderObjects) {
		entity.storePrevMatrix();
	}

//	m_gBuffer->toggleClear(true);
//	m_gBuffer->setClearColours(Colour::black());

	CommandBuffer graphicsBuffer = CommandBuffer::fromGraphics();

	graphicsBuffer.beginRendering(g_vulkanBackend->m_backbuffer);

	renderSkybox(graphicsBuffer, camera, g_vulkanBackend->m_backbuffer);
	renderObjects(graphicsBuffer, camera, g_vulkanBackend->m_backbuffer);

	graphicsBuffer.endRendering();
	graphicsBuffer.submit();

	renderImGui(graphicsBuffer);

//	renderParticles(camera, deltaTime);
//	g_vulkanBackend->beginGraphics();
//	renderPostProcess();
//	g_vulkanBackend->endGraphics();

	g_vulkanBackend->resetPushConstants();

	g_vulkanBackend->swapBuffers();
}

void Renderer::renderImGui(CommandBuffer &buffer)
{
	RenderInfo renderInfo;
	
	renderInfo.setSize(
		g_vulkanBackend->m_backbuffer->getWidth(),
		g_vulkanBackend->m_backbuffer->getHeight()
	);
	
	renderInfo.addColourAttachment(
		VK_ATTACHMENT_LOAD_OP_LOAD,
		g_vulkanBackend->m_backbuffer->getCurrentSwapchainImageView(),
		g_vulkanBackend->getImGuiAttachmentFormat(),
		VK_NULL_HANDLE
	);

	buffer.beginRendering(renderInfo);

	ImDrawData *drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, buffer.getBuffer());

	buffer.endRendering();
	buffer.submit();
}

void Renderer::renderParticles(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target, float deltaTime)
{
/*
	m_gBuffer->toggleClear(false);

	m_pushConstants.set("time", deltaTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);
	m_gpuParticles.dispatchCompute(camera);

	glm::mat4 particleMatrix = glm::scale(glm::identity<glm::mat4>(), { 0.1f, 0.1f, 0.1f });
	m_shaderParams.set("currModelMatrix", particleMatrix);
	m_shaderParams.set("prevModelMatrix", particleMatrix);
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_gpuParticles.render();
*/
}

void Renderer::renderPostProcess()
{
/*
	RenderPass pass;
	pass.setMesh(m_quadMesh);

	m_postProcessPipeline.bind();

	vkCmdBindDescriptorSets(
		g_vulkanBackend->graphicsQueue.getCurrentFrame().commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_postProcessPipeline.getPipelineLayout(),
		0,
		1, &m_postProcessDescriptorSet,
		0, nullptr
	);

	m_postProcessPipeline.render(pass);
*/
}

// todo: water. post processing pipeline.
