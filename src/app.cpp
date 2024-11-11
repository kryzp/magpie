#include "app.h"

#include "graphics/colour.h"

#include "platform.h"
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
	g_platform = new Platform(config);
	g_vulkanBackend = new VulkanBackend(config);

	g_inputState = new Input();

	m_backbuffer = g_vulkanBackend->createBackbuffer();
	m_backbuffer->setClearColour(0, Colour::black());

	g_platform->setWindowName(m_config.name);
	g_platform->setWindowSize({ m_config.width, m_config.height });
	g_platform->setWindowMode(m_config.windowMode);
	g_platform->setWindowOpacity(m_config.opacity);
	g_platform->toggleCursorVisible(!m_config.hasFlag(Config::FLAG_CURSOR_INVISIBLE));
	g_platform->toggleWindowResizable(m_config.hasFlag(Config::FLAG_RESIZABLE));

	if (m_config.hasFlag(Config::FLAG_CENTRE_WINDOW)) {
		const auto screenSize = g_platform->getScreenSize();
		g_platform->setWindowPosition({
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
	delete g_platform;
}

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

/*
struct Particle
{
	glm::vec2 position;
	glm::vec2 velocity;
};
*/

// note: yes, currently everything is just dumped into the app
//       this won't be the case after i abstract it a little, like splitting stuff off into particle systems
//       and custom "passes"
// 
//       or maybe i'll just leave it spaghettified. idk.

void App::run()
{
	ShaderProgram* vertexShaderInstanced	= g_shaderManager->create("../../res/shaders/raster/vertex_instanced.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* vertexShader				= g_shaderManager->create("../../res/shaders/raster/vertex.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* fragmentShader			= g_shaderManager->create("../../res/shaders/raster/fragment.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* fragmentShaderSkybox		= g_shaderManager->create("../../res/shaders/raster/fragment_skybox.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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

	RenderTarget* target = g_renderTargetManager->createTarget(m_backbuffer->getWidth(), m_backbuffer->getHeight(), { VK_FORMAT_B8G8R8A8_UNORM /*, VK_FORMAT_R8G8_UNORM*/ });
	TextureSampler* targetSampler = g_textureManager->getSampler("target", TextureSampler::Style(VK_FILTER_LINEAR));

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

	// default vertex format
	VertexDescriptor vertexFormat;
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	vertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
	vertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);

	// instanced vertex format
	VertexDescriptor instancedVertexFormat;
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	instancedVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
	instancedVertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);

	instancedVertexFormat.addAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyInstancedData, offset));
	instancedVertexFormat.addBinding(1, sizeof(MyInstancedData), VK_VERTEX_INPUT_RATE_INSTANCE);

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

	// setup instanced data
	const int INSTANCED_CUBE_COUNT = 256;
	uint64_t instanceDataSize = sizeof(MyInstancedData) * INSTANCED_CUBE_COUNT;
	MyInstancedData* instanceData = new MyInstancedData[INSTANCED_CUBE_COUNT];

	for (int i = 0; i < INSTANCED_CUBE_COUNT; i++)
	{
		float ii = (float)(i % 16);
		float jj = (float)(i / 16);

		ii *= 2.5f;
		jj *= 2.5f;

		instanceData[i].offset = { ii, 0.0f, -jj };
	}

	g_vulkanBackend->pushSsbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, instanceData, instanceDataSize);

	// setup camera
	Camera camera(m_config.width, m_config.height, 70.0f, 0.1f, 50.0f);
	camera.position = glm::vec3(0.0f, 4.0f, 6.0f);
	camera.setPitch(-glm::radians(30.0f));

	// shader parameters
	ShaderParameters pushConstants;
	pushConstants.set("time", 0.0f);

	glm::mat4 modelMatrix = glm::identity<glm::mat4>();

	ShaderParameters ubo;
	ubo.set("projMatrix", glm::identity<glm::mat4>());
	ubo.set("viewMatrix", glm::identity<glm::mat4>());
	ubo.set("modelMatrix", glm::identity<glm::mat4>());

	// delta time calculation
	double accumulator = 0.0;
	const double targetDeltaTime = 1.0 / static_cast<double>(m_config.targetFPS);
	double elapsedTime = 0.0;
	
	Timer elapsedTimer;
	elapsedTimer.start();

	RenderPass pass;

	auto getTransformationMatrix = [&](const glm::vec3& position, float rotationAngle, const glm::vec3& rotationAxis, const glm::vec3& scale, const glm::vec3& origin) -> glm::mat4 {
		return glm::translate(glm::identity<glm::mat4>(), position) *
			glm::rotate(glm::identity<glm::mat4>(), rotationAngle, rotationAxis) *
			glm::scale(glm::identity<glm::mat4>(), scale) *
			glm::translate(glm::identity<glm::mat4>(), -origin);
	};

	g_platform->setCursorPosition(m_config.width/2, m_config.height/2);

	while (m_running)
	{
		g_platform->pollEvents();
		g_inputState->update();

		if (g_inputState->isPressed(KB_KEY_ESCAPE)) {
			exit();
		}

		// --- ---

		double deltaTime = elapsedTimer.reset();

		accumulator += CalcF::min(deltaTime, targetDeltaTime);
		elapsedTime += deltaTime;

		while (accumulator >= targetDeltaTime)
		{
			camera.update(targetDeltaTime);
			g_platform->setCursorPosition(m_config.width/2, m_config.height/2);

			accumulator -= targetDeltaTime;
		}

		LLT_LOG("fps: %f", 1.0/deltaTime);

		// --- ---

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

		// --- ---

		g_vulkanBackend->setVertexDescriptor(vertexFormat);

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

		g_vulkanBackend->pushUbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);
		g_vulkanBackend->bindUbo(0, 0);

		g_vulkanBackend->setTexture(0, 1, skyboxTexture);
		g_vulkanBackend->setSampler(0, skyboxSampler);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentShaderSkybox);

		pass.mesh = &skybox;
		g_vulkanBackend->render(pass.build());

		// ---

		g_vulkanBackend->setDepthWrite(true);
		g_vulkanBackend->setDepthTest(true);

		ubo.set("viewMatrix", camera.getView());
		ubo.set("modelMatrix", glm::identity<glm::mat4>());
		g_vulkanBackend->pushUbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);
		g_vulkanBackend->bindUbo(0, 0);

		g_vulkanBackend->setTexture(0, 1, stoneTexture);
		g_vulkanBackend->setSampler(0, stoneSampler);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentShader);

		// floor
		ubo.set("modelMatrix", getTransformationMatrix({ 0.0f, 0.0f, 0.0f }, 0.0f, { 0.0f, 1.0f, 0.0f }, { 10.0f, 0.25f, 10.0f }, { 0.0f, 1.0f, 0.0f }));
		g_vulkanBackend->pushUbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);
		g_vulkanBackend->bindUbo(0, 0);

		pass.mesh = &block;
		g_vulkanBackend->render(pass.build());

		// block 1
		ubo.set("modelMatrix", getTransformationMatrix({ 1.0f, 0.0f, -5.0f }, (float)elapsedTime, {0.0f, 1.0f, 0.0}, {1.5f, 1.5f, 1.5f}, {0.0f, -1.0f, 0.0f}));
		g_vulkanBackend->pushUbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);
		g_vulkanBackend->bindUbo(0, 0);

		pass.mesh = &block;
		g_vulkanBackend->render(pass.build());

		// block 2
		ubo.set("modelMatrix", getTransformationMatrix({ -3.0f, 0.0f, 1.0f }, glm::radians(45.0f), { 0.0f, 1.0f, 0.0 }, { 0.75f, 0.75f, 0.75f }, { 0.0f, -1.0f, 0.0f }));
		g_vulkanBackend->pushUbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);
		g_vulkanBackend->bindUbo(0, 0);

		pass.mesh = &block;
		g_vulkanBackend->render(pass.build());

		// instancing!!!

		ubo.set("modelMatrix", glm::translate(glm::identity<glm::mat4>(), glm::vec3(5.0f, -5.0f, -5.0f)));
		g_vulkanBackend->pushUbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);
		g_vulkanBackend->bindUbo(0, 0);

		g_vulkanBackend->setVertexDescriptor(instancedVertexFormat);

		g_vulkanBackend->bindShader(vertexShaderInstanced);
		g_vulkanBackend->bindShader(fragmentShader);

		pass.instanceCount = INSTANCED_CUBE_COUNT;
		pass.instanceBuffer = g_vulkanBackend->getSsboBuffer(0);
		pass.mesh = &block;

		g_vulkanBackend->render(pass.build());

		pass.instanceCount = 1;
		pass.instanceBuffer = nullptr;

		g_vulkanBackend->endRender();

		// --- ---

		g_vulkanBackend->setVertexDescriptor(vertexFormat);

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

		g_vulkanBackend->pushUbo(0, VK_SHADER_STAGE_ALL_GRAPHICS, ubo);
		g_vulkanBackend->bindUbo(0, 0);

		g_vulkanBackend->setTexture(0, 1, target->getAttachment(0));
//		g_vulkanBackend->setTexture(0, 1, target->getDepthAttachment());
		g_vulkanBackend->setSampler(0, targetSampler);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentShader);

		pass.mesh = &quad;
		g_vulkanBackend->render(pass.build());

		g_vulkanBackend->endRender();

		// --- ---

		g_vulkanBackend->setTexture(0, 0, nullptr);
		g_vulkanBackend->setSampler(0, nullptr);

		g_vulkanBackend->resetPushConstants();

		// --- ---

		g_vulkanBackend->swapBuffers();
	}

	delete[] instanceData;
//	delete[] particleData;
}

void App::exit()
{
	m_running = false;

	if (m_config.onExit) {
		m_config.onExit();
	}
}
