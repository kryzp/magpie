#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#include "input/input.h"

#include "graphics/backend.h"
#include "graphics/backbuffer.h"
#include "graphics/texture.h"
#include "graphics/texture_mgr.h"
#include "graphics/render_target_mgr.h"
#include "graphics/material_system.h"
#include "graphics/mesh_loader.h"

#include "math/colour.h"

using namespace llt;

Renderer::Renderer()
	: m_backbuffer(nullptr)
	, m_quadMesh()
	, m_skyboxMesh()
//	, m_target(nullptr)
	, m_pushConstants()
//	, m_gpuParticles()
	, m_renderEntities()
	, m_prevViewMatrix()
	, m_skyboxMaterial()
//	, m_postProcessPipeline()
	, m_frameCount(0)
{
}

void Renderer::init(Backbuffer* backbuffer)
{
	m_backbuffer = backbuffer;
	m_pushConstants.setValue<float>("time", 0.0f);
	m_prevViewMatrix = glm::identity<glm::mat4>();

	initVertexTypes();
	createQuadMesh();
	loadTextures();

	g_materialSystem = new MaterialSystem();
	g_materialSystem->init();
	g_materialSystem->loadDefaultTechniques();

	g_meshLoader = new MeshLoader();

	createSkybox();

	auto backpack = m_renderEntities.emplaceBack();
	backpack->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	backpack->transform.setRotation(0.0f, { 0.0f, 1.0f, 0.0f });
	backpack->transform.setScale({ 1.0f, 1.0f, 1.0f });
	backpack->transform.setOrigin({ 0.0f, 0.0f, 0.0f });
	backpack->mesh = g_meshLoader->loadMesh("backpack", "../../res/models/backpack.obj");

//	m_target = g_renderTargetManager->createTarget(
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
//	m_target->createDepthAttachment();
//
//	m_gpuParticles.init(m_shaderParamsBuffer);
//
//	m_postProcessPipeline.setVertexDescriptor(g_modelVertex);
//	m_postProcessPipeline.setDepthTest(false);
//	m_postProcessPipeline.setDepthWrite(false);
//	m_postProcessPipeline.bindShader(g_shaderManager->get("vertex"));
//	m_postProcessPipeline.bindShader(g_shaderManager->get("fragment"));
//	m_postProcessPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
//	m_postProcessPipeline.bindBuffer(0, g_materialSystem->getGlobalBuffer());
//	m_postProcessPipeline.bindBuffer(1, g_materialSystem->getInstanceBuffer());
//	//m_postProcessPipeline.bindBuffer(2, nullptr);
//	m_postProcessPipeline.bindTexture(3, m_target->getAttachment(0), g_textureManager->getSampler("linear"));
}

void Renderer::cleanUp()
{
	delete g_meshLoader;
	delete g_materialSystem;
}

void Renderer::loadTextures()
{
	g_textureManager->createSampler("linear", TextureSampler::Style(VK_FILTER_LINEAR));
	g_textureManager->createSampler("nearest", TextureSampler::Style(VK_FILTER_NEAREST));

	g_textureManager->createCubeMap("skybox", VK_FORMAT_R8G8B8A8_UNORM,
		"../../res/textures/skybox/right.jpg",
		"../../res/textures/skybox/left.jpg",
		"../../res/textures/skybox/top.jpg",
		"../../res/textures/skybox/bottom.jpg",
		"../../res/textures/skybox/front.jpg",
		"../../res/textures/skybox/back.jpg"
	);

	g_textureManager->createFromImage("stone", Image("../../res/textures/smooth_stone.png"));
}

void Renderer::createQuadMesh()
{
	Vector<ModelVertex> quadVertices = {
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
	};

	Vector<uint16_t> quadIndices = {
		0, 1, 2,
		0, 2, 3
	};

	m_quadMesh.build(
		quadVertices.data(), quadVertices.size(),
		sizeof(ModelVertex),
		quadIndices.data(), quadIndices.size()
	);
}

void Renderer::createSkybox()
{
	Vector<ModelVertex> skyboxVertices = {
		{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } }
	};

	Vector<uint16_t> skyboxIndices = {
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
		skyboxVertices.data(), skyboxVertices.size(),
		sizeof(ModelVertex),
		skyboxIndices.data(), skyboxIndices.size()
	);

	MaterialData skyboxData;
	skyboxData.textures.pushBack({ g_textureManager->getTexture("skybox"), g_textureManager->getSampler("linear") });
	skyboxData.technique = "skybox";
	skyboxData.depthTest = false;
	skyboxData.depthWrite = false;

	m_skyboxMaterial = g_materialSystem->buildMaterial(skyboxData);
}

void Renderer::renderSkybox(const Camera& camera, float deltaTime, float elapsedTime)
{
	m_pushConstants.setValue<float>("time", elapsedTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);

	g_materialSystem->globalParameters.setValue<glm::mat4>("projMatrix", camera.getProj());
	g_materialSystem->globalParameters.setValue<glm::mat4>("viewMatrix", camera.getRotationMatrix());
	g_materialSystem->globalParameters.setValue<glm::vec4>("viewPos", { camera.position.x, camera.position.y, camera.position.z, 0.0f });
	g_materialSystem->updateGlobalBuffer();

	g_materialSystem->instanceParameters.setValue<glm::mat4>("modelMatrix", glm::identity<glm::mat4>());
	g_materialSystem->instanceParameters.setValue<glm::mat4>("normalMatrix", glm::identity<glm::mat4>());
	g_materialSystem->updateInstanceBuffer();

	RenderPass pass;
	pass.setMesh(m_skyboxMesh);

	m_skyboxMaterial->passes[SHADER_PASS_FORWARD].pipeline.bind();

	uint32_t dynamicOffsets[] = { g_materialSystem->getGlobalBuffer()->getDynamicOffset(), g_materialSystem->getInstanceBuffer()->getDynamicOffset(), 0}; // todo: dynamic offsets

	vkCmdBindDescriptorSets(
		g_vulkanBackend->graphicsQueue.getCurrentFrame().commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_skyboxMaterial->passes[SHADER_PASS_FORWARD].pipeline.getPipelineLayout(),
		0,
		1, &m_skyboxMaterial->passes[SHADER_PASS_FORWARD].set,
		3, dynamicOffsets
	);

	m_skyboxMaterial->passes[SHADER_PASS_FORWARD].pipeline.render(pass);
}

void Renderer::renderEntities(const Camera& camera, float deltaTime, float elapsedTime)
{
	m_pushConstants.setValue<float>("time", elapsedTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);

	g_materialSystem->globalParameters.setValue<glm::mat4>("projMatrix", camera.getProj());
	g_materialSystem->globalParameters.setValue<glm::mat4>("viewMatrix", camera.getView());
	g_materialSystem->globalParameters.setValue<glm::vec4>("viewPos", { camera.position.x, camera.position.y, camera.position.z, 0.0f });
	g_materialSystem->updateGlobalBuffer();

	for (Entity& entity : m_renderEntities)
	{
		if (!entity.mesh) {
			continue;
		}

		for (int i = 0; i < entity.mesh->getSubmeshCount(); i++)
		{
			SubMesh* mesh = entity.mesh->getSubmesh(i);
			Material* material = mesh->getMaterial();

			RenderPass pass;
			pass.setMesh(*mesh);

			g_materialSystem->instanceParameters.setValue<glm::mat4>("modelMatrix", entity.transform.getMatrix());
			g_materialSystem->instanceParameters.setValue<glm::mat4>("normalMatrix", glm::transpose(glm::inverse(entity.transform.getMatrix())));
			g_materialSystem->updateInstanceBuffer();

			material->passes[SHADER_PASS_FORWARD].pipeline.bind(); // todo: this should not happen per entity

			uint32_t dynamicOffsets[] = { g_materialSystem->getGlobalBuffer()->getDynamicOffset(), g_materialSystem->getInstanceBuffer()->getDynamicOffset(), 0}; // todo: dynamic offsets

			vkCmdBindDescriptorSets(
				g_vulkanBackend->graphicsQueue.getCurrentFrame().commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				material->passes[SHADER_PASS_FORWARD].pipeline.getPipelineLayout(),
				0,
				1, &material->passes[SHADER_PASS_FORWARD].set,
				3, dynamicOffsets
			);

			material->passes[SHADER_PASS_FORWARD].pipeline.render(pass);
		}
	}
}

void Renderer::render(const Camera& camera, float deltaTime, float elapsedTime)
{
	for (Entity& entity : m_renderEntities) {
		entity.storePrevMatrix();
	}

//	m_target->toggleClear(true);
//	m_target->setClearColours(Colour::black());

	g_vulkanBackend->beginGraphics(/*m_target*/);

	renderSkybox(camera, deltaTime, elapsedTime);
	renderEntities(camera, deltaTime, elapsedTime);

	g_vulkanBackend->endGraphics();

	/*
	m_target->toggleClear(false);

	 renderParticles(camera, deltaTime, elapsedTime);

	 g_vulkanBackend->beginGraphics();

	m_postProcessPipeline.bind();

	m_pushConstants.setFloat("time", elapsedTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);

	g_materialSystem->globalParameters.setMat4("projMatrix", glm::identity<glm::mat4>());
	g_materialSystem->globalParameters.setMat4("currViewMatrix", glm::identity<glm::mat4>());
	g_materialSystem->globalParameters.setMat4("prevViewMatrix", glm::identity<glm::mat4>());
	g_materialSystem->updateGlobalBuffer();

	g_materialSystem->instanceParameters.setMat4("currModelMatrix", glm::identity<glm::mat4>());
	g_materialSystem->updateInstanceBuffer();

	RenderPass pass;
	pass.setMesh(m_quadMesh);
	m_postProcessPipeline.render(pass);

	 g_vulkanBackend->endGraphics();
*/

	g_vulkanBackend->resetPushConstants();
	g_vulkanBackend->swapBuffers();

	m_prevViewMatrix = camera.getView();
	m_frameCount++;
}

/*
void Renderer::renderParticles(const Camera& camera, float deltaTime, float elapsedTime)
{
	m_pushConstants.set("time", deltaTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);
	m_gpuParticles.dispatchCompute(camera);

	glm::mat4 particleMatrix = glm::scale(glm::identity<glm::mat4>(), { 0.1f, 0.1f, 0.1f });
	m_shaderParams.set("currModelMatrix", particleMatrix);
	m_shaderParams.set("prevModelMatrix", particleMatrix);
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_gpuParticles.render();
}

void Renderer::createEntities()
{
	Vector<ModelVertex> blockVertices =
	{
		// front face
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },

		// right face
		{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },

		// back face
		{ {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },

		// left face
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },

		// top face
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },

		// bottom face
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
	};

	Vector<uint16_t> blockIndices =
	{
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23
	};

	MaterialData stoneMaterialData;
	stoneMaterialData.textures.pushBack({ g_textureManager->getTexture("stone"), g_textureManager->getSampler("nearest") });
	stoneMaterialData.technique = "entity";

	Material* stoneMaterial = g_materialSystem->buildMaterial(stoneMaterialData);

	auto floor = m_renderEntities.emplaceBack();
	floor->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	floor->transform.setRotation(0.0f, { 0.0f, 1.0f, 0.0f });
	floor->transform.setScale({ 10.0f, 0.25f, 10.0f });
	floor->transform.setOrigin({ 0.0f, 1.0f, 0.0f });

	SubMesh* s1 = floor->mesh.createSubmesh();
	s1->build(blockVertices.data(), blockVertices.size(), sizeof(ModelVertex), blockIndices.data(), blockIndices.size());
	s1->setMaterial(stoneMaterial);

	auto blockLarge = m_renderEntities.emplaceBack();
	blockLarge->transform.setPosition({ 1.0f, 0.0f, -5.0f });
	blockLarge->transform.setRotation(0.0f, { 0.0f, 1.0f, 0.0f });
	blockLarge->transform.setScale({ 1.5f, 1.5f, 1.5f });
	blockLarge->transform.setOrigin({ 0.0f, -1.0f, 0.0f });

	SubMesh* s2 = blockLarge->mesh.createSubmesh();
	s2->build(blockVertices.data(), blockVertices.size(), sizeof(ModelVertex), blockIndices.data(), blockIndices.size());
	s2->setMaterial(stoneMaterial);

	auto blockSmall = m_renderEntities.emplaceBack();
	blockSmall->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	blockSmall->transform.setRotation(glm::radians(45.0f), { 0.0f, 1.0f, 0.0f });
	blockSmall->transform.setScale({ 0.75f, 0.75f, 0.75f });
	blockSmall->transform.setOrigin({ 0.0f, -1.0f, 0.0f });

	SubMesh* s3 = blockSmall->mesh.createSubmesh();
	s3->build(blockVertices.data(), blockVertices.size(), sizeof(ModelVertex), blockIndices.data(), blockIndices.size());
	s3->setMaterial(stoneMaterial);
}
*/
