#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#include "graphics/backend.h"
#include "graphics/backbuffer.h"
#include "graphics/texture.h"
#include "graphics/texture_mgr.h"
#include "graphics/render_target.h"
#include "graphics/render_target_mgr.h"
#include "graphics/shader.h"
#include "graphics/shader_mgr.h"
#include "graphics/shader_buffer.h"
#include "graphics/shader_buffer_mgr.h"

using namespace llt;

struct MyVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 col;
	glm::vec3 norm;
};

struct MyInstancedData
{
	glm::vec3 offset;
};

Renderer::Renderer()
	: m_backbuffer(nullptr)
	, m_vertexShaderInstanced(nullptr)
	, m_vertexShader(nullptr)
	, m_fragmentShader(nullptr)
	, m_fragmentShaderSkybox(nullptr)
	, m_computeProgram(nullptr)
	, m_quadMesh()
	, m_blockMesh()
	, m_skyboxMesh()
	, m_skyboxTexture(nullptr)
	, m_stoneTexture(nullptr)
	, m_target(nullptr)
	, m_linearSampler(nullptr)
	, m_nearestSampler(nullptr)
	, m_vertexFormat()
	, m_instancedVertexFormat()
	, m_instanceBuffer(nullptr)
	, m_shaderParams()
	, m_shaderParamsBuffer(nullptr)
	, m_pushConstants()
{
}

Renderer::~Renderer()
{
}

void Renderer::init(Backbuffer* backbuffer)
{
	m_backbuffer = backbuffer;

	m_target = g_renderTargetManager->createTarget(
		m_backbuffer->getWidth(),
		m_backbuffer->getHeight(),
		{ VK_FORMAT_B8G8R8A8_UNORM /*, VK_FORMAT_R8G8_UNORM*/ }
	);

	loadTextures();
	loadShaders();
	createQuadMesh();
	createBlockMesh();
	createSkybox();
	setupVertexFormats();
	createInstanceData();
	setupShaderParameters();
}

void Renderer::loadTextures()
{
	m_linearSampler = g_textureManager->getSampler("linear", TextureSampler::Style(VK_FILTER_LINEAR));
	m_nearestSampler = g_textureManager->getSampler("nearest", TextureSampler::Style(VK_FILTER_NEAREST));

	m_skyboxTexture = g_textureManager->createCubeMap("skybox", VK_FORMAT_R8G8B8A8_UNORM,
		"../../res/textures/skybox/right.png",
		"../../res/textures/skybox/left.png",
		"../../res/textures/skybox/top.png",
		"../../res/textures/skybox/bottom.png",
		"../../res/textures/skybox/front.png",
		"../../res/textures/skybox/back.png"
	);

	m_stoneTexture = g_textureManager->createFromImage("stone", Image("../../res/textures/smooth_stone.png"));
}

void Renderer::loadShaders()
{
	m_vertexShaderInstanced		= g_shaderManager->create("../../res/shaders/raster/vertex_instanced.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_vertexShader				= g_shaderManager->create("../../res/shaders/raster/vertex.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_fragmentShader			= g_shaderManager->create("../../res/shaders/raster/fragment.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	m_fragmentShaderSkybox		= g_shaderManager->create("../../res/shaders/raster/fragment_skybox.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	m_computeProgram = g_shaderManager->create("../../res/shaders/compute/particles.spv", VK_SHADER_STAGE_COMPUTE_BIT);
}

void Renderer::setupShaderParameters()
{
	m_pushConstants.set("time", 0.0f);

	m_shaderParams.set("projMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("viewMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("modelMatrix", glm::identity<glm::mat4>());

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
	Vector<MyVertex> blockVertices = {

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

	Vector<uint16_t> blockIndices = {
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
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } }
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
	// default
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	m_vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
	m_vertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);

	// ---

	// instanced (per vertex)
	m_instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	m_instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	m_instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	m_instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
	m_instancedVertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);

	// instanced (per instance)
	m_instancedVertexFormat.addAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyInstancedData, offset));
	m_instancedVertexFormat.addBinding(1, sizeof(MyInstancedData), VK_VERTEX_INPUT_RATE_INSTANCE);
}

void Renderer::createInstanceData()
{
	uint64_t instanceDataSize = sizeof(MyInstancedData) * INSTANCED_CUBE_COUNT;
	MyInstancedData* instanceData = new MyInstancedData[INSTANCED_CUBE_COUNT];

	for (int i = 0; i < INSTANCED_CUBE_COUNT; i++)
	{
		float ii = (float)(i % INSTANCED_LENGTH);
		float jj = (float)((i / INSTANCED_LENGTH) % INSTANCED_LENGTH);
		float kk = (float)(i / (INSTANCED_LENGTH*INSTANCED_LENGTH));

		ii *= 2.5f;
		jj *= 2.5f;
		kk *= 2.5f;

		instanceData[i].offset = { ii, kk, -jj };
	}

	m_instanceBuffer = g_shaderBufferManager->createSSBO();
	m_instanceBuffer->pushData(instanceData, instanceDataSize);

	delete[] instanceData;
}

void Renderer::render(const Camera& camera, float elapsedTime)
{
	auto getTransformationMatrix = [&](const glm::vec3& position, float rotationAngle, const glm::vec3& rotationAxis, const glm::vec3& scale, const glm::vec3& origin) -> glm::mat4 {
		return glm::translate(glm::identity<glm::mat4>(), position) *
			glm::rotate(glm::identity<glm::mat4>(), rotationAngle, rotationAxis) *
			glm::scale(glm::identity<glm::mat4>(), scale) *
			glm::translate(glm::identity<glm::mat4>(), -origin);
	};

	RenderPass pass;

	g_vulkanBackend->setVertexDescriptor(m_vertexFormat);

	g_vulkanBackend->setCullMode(VK_CULL_MODE_BACK_BIT);

	g_vulkanBackend->setRenderTarget(m_target);
	g_vulkanBackend->beginRender();

	m_pushConstants.set("time", elapsedTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);

	m_shaderParams.set("projMatrix", camera.getProj());

	g_vulkanBackend->setDepthWrite(false);
	g_vulkanBackend->setDepthTest(false);

	m_shaderParams.set("viewMatrix", camera.getViewNoTranslation());
	m_shaderParams.set("modelMatrix", glm::identity<glm::mat4>());

	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_shaderParamsBuffer->bind(0);

	g_vulkanBackend->setTexture(1, m_skyboxTexture, m_linearSampler);

	g_vulkanBackend->bindShader(m_vertexShader);
	g_vulkanBackend->bindShader(m_fragmentShaderSkybox);

	pass.mesh = &m_skyboxMesh;
	g_vulkanBackend->render(pass.build());

	// ---

	g_vulkanBackend->setDepthWrite(true);
	g_vulkanBackend->setDepthTest(true);

	m_shaderParams.set("viewMatrix", camera.getView());
	m_shaderParams.set("modelMatrix", glm::identity<glm::mat4>());

	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_shaderParamsBuffer->bind(0);

	g_vulkanBackend->setTexture(1, m_stoneTexture, m_nearestSampler);

	g_vulkanBackend->bindShader(m_vertexShader);
	g_vulkanBackend->bindShader(m_fragmentShader);

	// floor
	m_shaderParams.set("modelMatrix", getTransformationMatrix({ 0.0f, 0.0f, 0.0f }, 0.0f, { 0.0f, 1.0f, 0.0f }, { 10.0f, 0.25f, 10.0f }, { 0.0f, 1.0f, 0.0f }));
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_shaderParamsBuffer->bind(0);

	pass.mesh = &m_blockMesh;
	g_vulkanBackend->render(pass.build());

	// block 1
	m_shaderParams.set("modelMatrix", getTransformationMatrix({ 1.0f, 0.0f, -5.0f }, (float)elapsedTime, {0.0f, 1.0f, 0.0}, {1.5f, 1.5f, 1.5f}, {0.0f, -1.0f, 0.0f}));
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_shaderParamsBuffer->bind(0);

	pass.mesh = &m_blockMesh;
	g_vulkanBackend->render(pass.build());

	// block 2
	m_shaderParams.set("modelMatrix", getTransformationMatrix({ -3.0f, 0.0f, 1.0f }, glm::radians(45.0f), { 0.0f, 1.0f, 0.0 }, { 0.75f, 0.75f, 0.75f }, { 0.0f, -1.0f, 0.0f }));
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_shaderParamsBuffer->bind(0);

	pass.mesh = &m_blockMesh;
	g_vulkanBackend->render(pass.build());

	// instancing!!!

	m_shaderParams.set("modelMatrix", getTransformationMatrix({ 5.0f, -5.0f, -5.0f }, 0.0f, { 0.0f, 1.0f, 0.0 }, { 0.75f, 0.75f, 0.75f }, { 0.0f, 0.0f, 0.0f }));
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_shaderParamsBuffer->bind(0);

	g_vulkanBackend->setVertexDescriptor(m_instancedVertexFormat);

	g_vulkanBackend->bindShader(m_vertexShaderInstanced);
	g_vulkanBackend->bindShader(m_fragmentShader);

	pass.instanceCount = INSTANCED_CUBE_COUNT;
	pass.instanceBuffer = m_instanceBuffer->getBuffer();
	pass.mesh = &m_blockMesh;

	g_vulkanBackend->render(pass.build());

	pass.instanceCount = 1;
	pass.instanceBuffer = nullptr;

	g_vulkanBackend->endRender();

	// --- ---

	g_vulkanBackend->setVertexDescriptor(m_vertexFormat);

	g_vulkanBackend->beginRender();

	g_vulkanBackend->setDepthWrite(true);
	g_vulkanBackend->setDepthTest(false);

	g_vulkanBackend->setRenderTarget(m_backbuffer);
	g_vulkanBackend->beginRender();

	m_pushConstants.set("time", elapsedTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);

	m_shaderParams.set("projMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("viewMatrix", glm::identity<glm::mat4>());
	m_shaderParams.set("modelMatrix", glm::identity<glm::mat4>());

	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_shaderParamsBuffer->bind(0);

	g_vulkanBackend->setTexture(1, m_target->getAttachment(0), m_linearSampler);
	//g_vulkanBackend->setTexture(1, target->getDepthAttachment(), m_linearSampler);

	g_vulkanBackend->bindShader(m_vertexShader);
	g_vulkanBackend->bindShader(m_fragmentShader);

	pass.mesh = &m_quadMesh;
	g_vulkanBackend->render(pass.build());

	g_vulkanBackend->endRender();

	// --- ---

	g_vulkanBackend->setTexture(0, nullptr, nullptr);

	g_vulkanBackend->resetPushConstants();

	// --- ---

	g_vulkanBackend->swapBuffers();
}


/*
g_vulkanBackend->beginCompute();

g_vulkanBackend->bindShader(computeProgram);

parameters.set("deltaTime", (float)targetDeltaTime);
g_vulkanBackend->pushUbo(0, 0, VK_SHADER_STAGE_COMPUTE_BIT, parameters);

g_vulkanBackend->bindSsbo(0, 1);

g_vulkanBackend->dispatchCompute(1, 1, 1);

g_vulkanBackend->endCompute();

// ---

RENDERING TO FBO

// ---

g_vulkanBackend->syncComputeWithNextRender();

// ---

RENDER FBO ON QUAD TO BACKBUFFER
*/

/*
struct Particle
{
	glm::vec2 position;
	glm::vec2 velocity;
};
*/

/*
int nParticles = 8;
uint64_t particleBufferSize = sizeof(Particle) * nParticles;
Particle* particleData = new Particle[nParticles];

for (int i = 0; i < nParticles; i++)
{
particleData[i].position = { i, 0.0f };
particleData[i].velocity = { 0.0f, i * 0.05f };
}

MyInstancedData instanceRawData[8];

for (int i = 0; i < 8; i++)
{
instanceRawData[i].positionOffset.x = i;
instanceRawData[i].positionOffset.y = 0.0f;
}

GPUBuffer* instanceData = g_gpuBufferManager->createVertexBuffer(8, sizeof(MyInstancedData));
GPUBuffer* instanceDataStage = g_gpuBufferManager->createStagingBuffer(sizeof(MyInstancedData) * 8);
instanceDataStage->writeDataToMe(instanceRawData, sizeof(MyInstancedData) * 8, 0);
instanceDataStage->writeToBuffer(instanceData, sizeof(MyInstancedData) * 8, 0, 0);
delete instanceDataStage;

ShaderParameters parameters;
parameters.set("deltaTime", 1.0f);

ShaderParameters parameters2;
parameters2.set("otherData", 1.0f);

g_vulkanBackend->pushSsbo(0, 1, VK_SHADER_STAGE_COMPUTE_BIT, particleData, particleBufferSize);

delete[] particleData;
*/
