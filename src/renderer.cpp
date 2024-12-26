#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#include "input/input.h"

#include "graphics/backend.h"
#include "graphics/backbuffer.h"
#include "graphics/colour.h"
#include "graphics/texture.h"
#include "graphics/texture_mgr.h"
#include "graphics/render_target_mgr.h"
#include "graphics/shader.h"
#include "graphics/shader_mgr.h"
#include "graphics/shader_buffer_mgr.h"

using namespace llt;

Renderer::Renderer()
	: m_backbuffer(nullptr)
	, m_vertexShaderInstanced(nullptr)
	, m_vertexShader(nullptr)
	, m_fragmentShader(nullptr)
	, m_fragmentShaderSkybox(nullptr)
	, m_quadMesh()
	, m_blockMesh()
	, m_skyboxMesh()
	, m_skyboxTexture(nullptr)
	, m_stoneTexture(nullptr)
	, m_target(nullptr)
	, m_linearSampler(nullptr)
	, m_nearestSampler(nullptr)
	, m_vertexFormat()
	, m_shaderParams()
	, m_shaderParamsBuffer(nullptr)
	, m_pushConstants()
	, m_gpuParticles()
	, m_renderEntities()
	, m_prevViewMatrix()
	, m_skyboxPipeline()
	, m_entityPipeline()
	, m_postProcessPipeline()
	, m_frames(0)
{
}

Renderer::~Renderer()
{
}

void Renderer::init(Backbuffer* backbuffer)
{
	m_backbuffer = backbuffer;

	m_target = g_renderTargetManager->createTarget(
		"gBuffer", // YES I KNOW THIS ISNT A DEFERRED RENDERING GBUFFER BUT WHATEVER
		m_backbuffer->getWidth(),
		m_backbuffer->getHeight(),
		{
			VK_FORMAT_B8G8R8A8_UNORM, // colour
			VK_FORMAT_R32G32_SFLOAT, // motion
			VK_FORMAT_R32G32B32A32_SFLOAT // normals
		}
	);

	loadTextures();
	loadShaders();
	createQuadMesh();
	createBlockMesh();
	createSkybox();
	setupVertexFormats();
	setupShaderParameters();
	createEntities();

	m_prevViewMatrix = glm::identity<glm::mat4>();

	m_gpuParticles.init(m_shaderParamsBuffer);

	createPipelines();
}

void Renderer::loadTextures()
{
	m_linearSampler = g_textureManager->createSampler("linear", TextureSampler::Style(VK_FILTER_LINEAR));
	m_nearestSampler = g_textureManager->createSampler("nearest", TextureSampler::Style(VK_FILTER_NEAREST));

	m_skyboxTexture = g_textureManager->createCubeMap("skybox", VK_FORMAT_R8G8B8A8_UNORM,
		"../res/textures/skybox/right.png",
		"../res/textures/skybox/left.png",
		"../res/textures/skybox/top.png",
		"../res/textures/skybox/bottom.png",
		"../res/textures/skybox/front.png",
		"../res/textures/skybox/back.png"
	);

	m_stoneTexture = g_textureManager->createFromImage("stone", Image("../res/textures/smooth_stone.png"));
}

void Renderer::loadShaders()
{
	m_vertexShaderInstanced		= g_shaderManager->create("vertexInstanced", "../res/shaders/raster/vertex_instanced.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_vertexShader				= g_shaderManager->create("vertex", "../res/shaders/raster/vertex.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_fragmentShader			= g_shaderManager->create("fragment", "../res/shaders/raster/fragment.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	m_fragmentShaderSkybox		= g_shaderManager->create("fragmentSkybox", "../res/shaders/raster/fragment_skybox.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void Renderer::setupShaderParameters()
{
	m_pushConstants.set("time", 0.0f);

	m_shaderParams.set("projMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("currViewMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("currModelMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("prevModelMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("prevViewMatrix", glm::identity<glm::mat4>());

	m_shaderParamsBuffer = g_shaderBufferManager->createUBO();
}

void Renderer::createQuadMesh()
{
	Vector<MyVertex> quadVertices = {
		{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
	};

	Vector<uint16_t> quadIndices = {
		0, 1, 2,
		0, 2, 3
	};

	m_quadMesh.build(quadVertices.data(), quadVertices.size(), sizeof(MyVertex), quadIndices.data(), quadIndices.size());
}

void Renderer::createBlockMesh()
{
	Vector<MyVertex> blockVertices =
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

	m_blockMesh.build(blockVertices.data(), blockVertices.size(), sizeof(MyVertex), blockIndices.data(), blockIndices.size());
}

void Renderer::createSkybox()
{
	Vector<MyVertex> skyboxVertices = {
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

	m_skyboxMesh.build(skyboxVertices.data(), skyboxVertices.size(), sizeof(MyVertex), skyboxIndices.data(), skyboxIndices.size());
}

void Renderer::setupVertexFormats()
{
	m_vertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
}

void Renderer::createEntities()
{
	auto floor = m_renderEntities.emplaceBack();
	floor->setPosition({ 0.0f, 0.0f, 0.0f });
	floor->setRotation(0.0f, { 0.0f, 1.0f, 0.0f });
	floor->setScale({ 10.0f, 0.25f, 10.0f });
	floor->setOrigin({ 0.0f, 1.0f, 0.0f });

	auto blockLarge = m_renderEntities.emplaceBack();
	blockLarge->setPosition({ 1.0f, 0.0f, -5.0f });
	blockLarge->setRotation(0.0f, { 0.0f, 1.0f, 0.0f });
	blockLarge->setScale({ 1.5f, 1.5f, 1.5f });
	blockLarge->setOrigin({ 0.0f, -1.0f, 0.0f });

	auto blockSmall = m_renderEntities.emplaceBack();
	blockSmall->setPosition({ 0.0f, 0.0f, 0.0f });
	blockSmall->setRotation(glm::radians(45.0f), { 0.0f, 1.0f, 0.0f });
	blockSmall->setScale({ 0.75f, 0.75f, 0.75f });
	blockSmall->setOrigin({ 0.0f, -1.0f, 0.0f });
}

void Renderer::createPipelines()
{
	m_skyboxPipeline.setVertexDescriptor(m_vertexFormat);
	m_skyboxPipeline.setDepthTest(false);
	m_skyboxPipeline.setDepthWrite(false);
	m_skyboxPipeline.bindShader(m_vertexShader);
	m_skyboxPipeline.bindShader(m_fragmentShaderSkybox);
	m_skyboxPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_skyboxPipeline.bindBuffer(0, m_shaderParamsBuffer);
	m_skyboxPipeline.bindTexture(1, m_skyboxTexture, m_linearSampler);

	m_entityPipeline.setVertexDescriptor(m_vertexFormat);
	m_entityPipeline.setDepthTest(true);
	m_entityPipeline.setDepthWrite(true);
	m_entityPipeline.bindShader(m_vertexShader);
	m_entityPipeline.bindShader(m_fragmentShader);
	m_entityPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_entityPipeline.bindBuffer(0, m_shaderParamsBuffer);
	m_entityPipeline.bindTexture(1, m_stoneTexture, m_nearestSampler);

	m_postProcessPipeline.setVertexDescriptor(m_vertexFormat);
	m_postProcessPipeline.setDepthTest(false);
	m_postProcessPipeline.setDepthWrite(false);
	m_postProcessPipeline.bindShader(m_vertexShader);
	m_postProcessPipeline.bindShader(m_fragmentShader);
	m_postProcessPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_postProcessPipeline.bindBuffer(0, m_shaderParamsBuffer);
	m_postProcessPipeline.bindTexture(1, m_target->getAttachment(0), m_linearSampler);
}

void Renderer::render(const Camera& camera, float deltaTime, float elapsedTime)
{
	for (Entity& entity : m_renderEntities) {
		entity.storePrevMatrix();
	}

	m_renderEntities.back().setRotation(elapsedTime * 2.0f, { 0.0f, 1.0f, 0.0f });

	glm::mat4 viewMatrix = camera.getView();

	m_target->toggleClear(true);
	m_target->setClearColours(Colour::black());

	RenderOp pass;

	g_vulkanBackend->beginGraphics(m_target);

	m_pushConstants.set("time", elapsedTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);

	m_shaderParams.set("projMatrix", camera.getProj());
	m_shaderParams.set("currViewMatrix", camera.getViewNoTranslation());
	m_shaderParams.set("currModelMatrix", glm::identity<glm::mat4>());
	m_shaderParamsBuffer->pushData(m_shaderParams);

	pass.setMesh(m_skyboxMesh);

	m_skyboxPipeline.bind();
	m_skyboxPipeline.render(pass);

	m_entityPipeline.bind();

	m_shaderParams.set("prevViewMatrix", m_prevViewMatrix);
	m_shaderParams.set("currViewMatrix", viewMatrix);

	pass.setMesh(m_blockMesh);

	for (Entity& entity : m_renderEntities)
	{
		m_shaderParams.set("prevModelMatrix", entity.getPrevMatrix());
		m_shaderParams.set("currModelMatrix", entity.getMatrix());
		m_shaderParamsBuffer->pushData(m_shaderParams);

		m_entityPipeline.render(pass);
	}

	g_vulkanBackend->endGraphics();
}

void Renderer::renderPostCamera(const Camera& camera, float deltaTime, float elapsedTime)
{
	glm::mat4 viewMatrix = camera.getView();

	RenderOp pass;

	m_target->toggleClear(false);

	if (m_frames >= 10)
	{
		// particles: compute
		m_pushConstants.set("time", deltaTime);
		g_vulkanBackend->setPushConstants(m_pushConstants);
		m_gpuParticles.dispatchCompute(camera);

		// particles: render
		glm::mat4 particleMatrix = glm::scale(glm::identity<glm::mat4>(), { 0.1f, 0.1f, 0.1f });
		m_shaderParams.set("currModelMatrix", particleMatrix);
		m_shaderParams.set("prevModelMatrix", particleMatrix);
		m_shaderParamsBuffer->pushData(m_shaderParams);
		m_gpuParticles.render();
	}

	g_vulkanBackend->beginGraphics();

	m_postProcessPipeline.bind();

	m_pushConstants.set("time", elapsedTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);

	m_shaderParams.set("projMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("currViewMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("currModelMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("prevModelMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("prevViewMatrix", glm::identity<glm::mat4>());
	m_shaderParamsBuffer->pushData(m_shaderParams);

	pass.setMesh(m_quadMesh);
	m_postProcessPipeline.render(pass);

	g_vulkanBackend->endGraphics();

	g_vulkanBackend->resetPushConstants();
	g_vulkanBackend->swapBuffers();

	m_frames++;

	m_prevViewMatrix = viewMatrix;
}
