#include "app.h"

#include "graphics/colour.h"

#include "system_backend.h"
#include "common.h"
#include "camera.h"
#include "input/input.h"
#include "graphics/backend.h"
#include "graphics/render_target.h"
#include "math/timer.h"
#include "math/calc.h"

#include <glm/gtc/matrix_transform.hpp>

llt::App* llt::g_app = nullptr;

using namespace llt;

App::App(const Config& config)
	: m_config(config)
	, m_running(false)
	, m_backbuffer(nullptr)
{
	g_systemBackend = new SystemBackend(config);
	g_vulkanBackend = new VulkanBackend(config);

	g_inputState = new Input();

	m_backbuffer = g_vulkanBackend->createBackbuffer();
	m_backbuffer->setClearColour(0, Colour::black());

	g_systemBackend->setWindowName(m_config.name);
	g_systemBackend->setWindowSize({ m_config.width, m_config.height });
	g_systemBackend->setWindowMode(m_config.windowMode);
	g_systemBackend->setWindowOpacity(m_config.opacity);
	g_systemBackend->toggleCursorVisible(!m_config.hasFlag(Config::FLAG_CURSOR_INVISIBLE));
	g_systemBackend->toggleWindowResizable(m_config.hasFlag(Config::FLAG_RESIZABLE));

	if (m_config.hasFlag(Config::FLAG_CENTRE_WINDOW)) {
		const auto screenSize = g_systemBackend->getScreenSize();
		g_systemBackend->setWindowPosition({
			(int)(screenSize.x - m_config.width) / 2,
			(int)(screenSize.y - m_config.height) / 2
		});
	}

	m_running = true;

	if (m_config.onInit) {
		m_config.onInit();
	}
}

App::~App()
{
	if (m_config.onDestroy) {
		m_config.onDestroy();
	}

	delete g_inputState;

	delete g_vulkanBackend;
	delete g_systemBackend;
}

struct MyVertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 col;
	glm::vec3 norm;
};

/*
struct Particle
{
	glm::vec2 position;
	glm::vec2 velocity;
};

struct MyInstancedData
{
	glm::vec3 positionOffset;
};
*/

// note: yes, currently everything is just dumped into the app
//       this won't be the case after i abstract it a little, like splitting stuff off into particle systems
//       and custom "passes"
// 
//       or maybe i'll just leave it spaghettified. idk.

void App::run()
{
	ShaderProgram* vertexShaderInst		= g_shaderManager->create("../../res/shaders/raster/vertex_instanced.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* vertexShader			= g_shaderManager->create("../../res/shaders/raster/vertex.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* fragmentShader		= g_shaderManager->create("../../res/shaders/raster/fragment.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* fragmentShaderSkybox = g_shaderManager->create("../../res/shaders/raster/fragment_skybox.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	ShaderProgram* computeProgram = g_shaderManager->create("../../res/shaders/compute/particles.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	Texture* skyboxTexture = g_textureManager->createCubeMap("skybox", VK_FORMAT_R8G8B8A8_UNORM,
		"../../res/textures/skybox/right.png",
		"../../res/textures/skybox/left.png",
		"../../res/textures/skybox/top.png",
		"../../res/textures/skybox/bottom.png",
		"../../res/textures/skybox/front.png",
		"../../res/textures/skybox/back.png"
	);
	TextureSampler* skyboxSampler = g_textureManager->getSampler("skybox", TextureSampler::Style(VK_FILTER_NEAREST));

	Texture* stoneTexture = g_textureManager->createFromImage("stone", Image("../../res/textures/smooth_stone.png"));
	TextureSampler* stoneSampler = g_textureManager->getSampler("stone", TextureSampler::Style(VK_FILTER_NEAREST));

	RenderTarget* target = g_renderTargetManager->createTarget(1280, 720, { VK_FORMAT_B8G8R8A8_UNORM /*, VK_FORMAT_R8G8_UNORM*/ });
	TextureSampler* targetSampler = g_textureManager->getSampler("target", TextureSampler::Style(VK_FILTER_LINEAR));

	VertexDescriptor vertexFormat;
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
	vertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);

	g_vulkanBackend->setVertexDescriptor(vertexFormat);

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

	SubMesh quad;
	quad.build(quadVertices.data(), quadVertices.size(), sizeof(MyVertex), quadIndices.data(), quadIndices.size());

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

	SubMesh block;
	block.build(blockVertices.data(), blockVertices.size(), sizeof(MyVertex), blockIndices.data(), blockIndices.size());

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

	SubMesh skybox;
	skybox.build(skyboxVertices.data(), skyboxVertices.size(), sizeof(MyVertex), skyboxIndices.data(), skyboxIndices.size());

	/*
	VertexDescriptor instancedVertexFormat;

	// regular data
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
	instancedVertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);

	// instanced component
//	instancedVertexFormat.addAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, 0);
	instancedVertexFormat.addAttribute(1, VK_FORMAT_R32G32_SFLOAT, 0);
	instancedVertexFormat.addBinding(1, sizeof(Particle), VK_VERTEX_INPUT_RATE_INSTANCE);
//	instancedVertexFormat.addBinding(1, sizeof(MyInstancedData), VK_VERTEX_INPUT_RATE_INSTANCE);

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

	GPUBuffer* instanceData = g_bufferManager->createVertexBuffer(8, sizeof(MyInstancedData));
	GPUBuffer* instanceDataStage = g_bufferManager->createStagingBuffer(sizeof(MyInstancedData) * 8);
	instanceDataStage->writeDataToMe(instanceRawData, sizeof(MyInstancedData) * 8, 0);
	instanceDataStage->writeToBuffer(instanceData, sizeof(MyInstancedData) * 8, 0, 0);
	delete instanceDataStage;

	ShaderParameters parameters;
	parameters.set("deltaTime", 1.0f);

	ShaderParameters parameters2;
	parameters2.set("otherData", 1.0f);

	g_vulkanBackend->pushSsbo(0, 1, VK_SHADER_STAGE_COMPUTE_BIT, particleData, particleBufferSize);
	*/

	Camera camera(m_config.width, m_config.height, 70.0f, 0.1f, 10.0f);

	ShaderParameters pushConstants;
	pushConstants.set("time", 0.0f);

	ShaderParameters ubo;
	ubo.set("projMatrix", glm::identity<glm::mat4>());
	ubo.set("viewMatrix", glm::identity<glm::mat4>());
	ubo.set("modelMatrix", glm::identity<glm::mat4>());

	double elapsedTime = 0.0;
	uint64_t lastPerformanceCounter = g_systemBackend->getPerformanceCounter();
	double accumulator = 0.0;
	const double targetDeltaTime = 1.0 / static_cast<double>(m_config.targetFPS);

	RenderPass pass;

	while (m_running)
	{
		g_systemBackend->pollEvents();
		g_inputState->update();

		if (g_inputState->isPressed(KB_KEY_ESCAPE)) {
			exit();
		}

		// ---

		uint64_t currentPerformanceCounter = g_systemBackend->getPerformanceCounter();

		double deltaTime = static_cast<double>(currentPerformanceCounter - lastPerformanceCounter) / static_cast<double>(g_systemBackend->getPerformanceFrequency());
		lastPerformanceCounter = currentPerformanceCounter;

		accumulator += CalcF::min(deltaTime, targetDeltaTime);
		elapsedTime += deltaTime;

		while (accumulator >= targetDeltaTime)
		{
			camera.update(targetDeltaTime);
			g_systemBackend->setCursorPosition(1280/2, 720/2);

			accumulator -= targetDeltaTime;
		}

		// ---

		/*
		
		g_vulkanBackend->beginCompute();

		g_vulkanBackend->bindShader(computeProgram);

		parameters.set("deltaTime", (float)targetDeltaTime);
		g_vulkanBackend->pushUbo(0, 0, VK_SHADER_STAGE_COMPUTE_BIT, parameters);

		g_vulkanBackend->bindSsbo(0, 1);

		g_vulkanBackend->dispatchCompute(1, 1, 1);
		
		g_vulkanBackend->endCompute();

		// ---

		g_vulkanBackend->setDepthTest(true);

		g_vulkanBackend->setVertexDescriptor(vertexFormat);

		g_vulkanBackend->setCullMode(VK_CULL_MODE_BACK_BIT);

		g_vulkanBackend->setRenderTarget(target);
		g_vulkanBackend->beginRender();

		pushConstants.set("projMatrix", glm::perspective(70.0f, aspect, 0.1f, 10.0f));
		pushConstants.set("modelMatrix", glm::translate(glm::identity<glm::mat4>(), glm::vec3(1.0f, -1.0f, 0.0f)));
		g_vulkanBackend->setPushConstants(pushConstants);

		pass.mesh = &mesh;

		g_vulkanBackend->bindShader(fragmentShader);

		g_vulkanBackend->setVertexDescriptor(instancedVertexFormat);
		g_vulkanBackend->bindShader(vertexShaderInst);
		pass.instanceCount = nParticles;
		pass.instanceBuffer = g_vulkanBackend->getSsboBuffer(0);
		g_vulkanBackend->render(pass.build());
		pass.instanceCount = 1;
		pass.instanceBuffer = nullptr;

		g_vulkanBackend->setVertexDescriptor(vertexFormat);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->render(pass.build());

		g_vulkanBackend->endRender();

		// ---

		g_vulkanBackend->syncComputeWithNextRender();
		
		// ---

		g_vulkanBackend->setDepthTest(false);

		g_vulkanBackend->setRenderTarget(m_backbuffer);
		g_vulkanBackend->beginRender();

		pushConstants.set("projMatrix", glm::identity<glm::mat4>());
		pushConstants.set("modelMatrix", glm::identity<glm::mat4>());
		g_vulkanBackend->setPushConstants(pushConstants);

		parameters.set("deltaTime", 1.0f);
		g_vulkanBackend->pushUbo(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, parameters);
		parameters2.set("otherData", 1.0f);
		g_vulkanBackend->pushUbo(1, 1, VK_SHADER_STAGE_ALL_GRAPHICS, parameters2);

		g_vulkanBackend->bindSsbo(0, 2);

		g_vulkanBackend->setTexture(0, target->getAttachment(0));
		g_vulkanBackend->setSampler(0, targetSampler);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentTexShader);

		pass.mesh = &quad;
		g_vulkanBackend->render(pass.build());

		pass.mesh = &quad2;
		g_vulkanBackend->render(pass.build());

		g_vulkanBackend->endRender();

		// ---

		g_vulkanBackend->setTexture(0, nullptr);
		g_vulkanBackend->setSampler(0, nullptr);

		g_vulkanBackend->resetPushConstants();

		*/

		// ---

		g_vulkanBackend->setCullMode(VK_CULL_MODE_BACK_BIT);

		g_vulkanBackend->setRenderTarget(target);
		g_vulkanBackend->beginRender();

		pushConstants.set("time", (float)elapsedTime);
		g_vulkanBackend->setPushConstants(pushConstants);

		ubo.set("projMatrix", camera.getProj());

		g_vulkanBackend->setDepthWrite(false);
		g_vulkanBackend->setDepthTest(false);

		ubo.set("viewMatrix", camera.getViewNoTranslation());
		ubo.set("modelMatrix", glm::identity<glm::mat4>());

		g_vulkanBackend->pushUbo(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);

		g_vulkanBackend->setTexture(0, skyboxTexture);
		g_vulkanBackend->setSampler(0, skyboxSampler);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentShaderSkybox);

		pass.mesh = &skybox;
		g_vulkanBackend->render(pass.build());

		g_vulkanBackend->setDepthWrite(true);
		g_vulkanBackend->setDepthTest(true);

		glm::mat4 model = glm::identity<glm::mat4>();
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f));
		model = glm::rotate(model, (float)elapsedTime, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, (float)elapsedTime, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, (float)elapsedTime, glm::vec3(0.0f, 0.0f, -1.0f));

		ubo.set("viewMatrix", camera.getView());
		ubo.set("modelMatrix", model);

		g_vulkanBackend->pushUbo(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);

		g_vulkanBackend->setTexture(0, stoneTexture);
		g_vulkanBackend->setSampler(0, stoneSampler);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentShader);

		pass.mesh = &block;
		g_vulkanBackend->render(pass.build());

		g_vulkanBackend->endRender();

		// ---

		g_vulkanBackend->beginRender();

		g_vulkanBackend->setDepthWrite(true);
		g_vulkanBackend->setDepthTest(false);

		g_vulkanBackend->setRenderTarget(m_backbuffer);
		g_vulkanBackend->beginRender();

		pushConstants.set("time", (float)elapsedTime);
		g_vulkanBackend->setPushConstants(pushConstants);

		ubo.set("projMatrix", glm::identity<glm::mat4>());
		ubo.set("viewMatrix", glm::identity<glm::mat4>());
		ubo.set("modelMatrix", glm::identity<glm::mat4>());

		g_vulkanBackend->pushUbo(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);

		g_vulkanBackend->setTexture(0, target->getAttachment(0));
		g_vulkanBackend->setSampler(0, targetSampler);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentShader);

		pass.mesh = &quad;
		g_vulkanBackend->render(pass.build());

		g_vulkanBackend->endRender();

		// ---

		g_vulkanBackend->setTexture(0, nullptr);
		g_vulkanBackend->setSampler(0, nullptr);

		g_vulkanBackend->resetPushConstants();

		// ---

		g_vulkanBackend->swapBuffers();
	}

//	delete[] particleData;
}

void App::exit()
{
	m_running = false;

	if (m_config.onExit) {
		m_config.onExit();
	}
}
