#include "app.h"

#include "graphics/colour.h"

#include "system_backend.h"
#include "common.h"
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

struct Particle
{
	glm::vec2 position;
	glm::vec2 velocity;
};

void App::run()
{
	ShaderParameters pushConstants;

	ShaderProgram* vertexShader			= g_shaderManager->create("../../res/shaders/raster/vertex.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram* fragmentShader		= g_shaderManager->create("../../res/shaders/raster/fragment.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	ShaderProgram* fragmentTexShader	= g_shaderManager->create("../../res/shaders/raster/fragment_texture.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	ShaderProgram* computeProgram = g_shaderManager->create("../../res/shaders/compute/particles.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	RenderTarget* target = g_renderTargetManager->createTarget(1280, 720, { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8_UNORM });
	TextureSampler* sampler = g_textureManager->getSampler("asdf", TextureSampler::Style());

	SubMesh mesh;
	mesh.build(
		{
			{ { -1.0f, -1.0f, -5.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
			{ {  1.0f, -1.0f, -5.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { -1.0f,  1.0f, -5.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ {  1.0f,  1.0f, -5.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
		},
		{
			0, 1, 2,
			1, 3, 2
		}
	);

	SubMesh quad;
	quad.build(
		{
			{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
		},
		{
			0, 1, 2,
			1, 3, 2
		}
	);

	SubMesh quad2;
	quad2.build(
		{
			{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
			{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }
		},
		{
			0, 1, 2,
			1, 3, 2
		}
	);

	RenderPass pass;

	int nParticles = 8;
	uint64_t particleBufferSize = sizeof(Particle) * nParticles;
	Particle* particleData = new Particle[nParticles];

	for (int i = 0; i < nParticles; i++)
	{
		particleData[i].position = { 1.0f, 0.0f };
		particleData[i].velocity = { 0.0f, 0.0f };
	}

	ShaderParameters parameters;
	parameters.set("time", 1.0f);

	ShaderParameters parameters2;
	parameters2.set("otherData", 1.0f);

	Timer deltaTimer;
	deltaTimer.start();

	double accumulator = 0.0;
	const double deltaTime = 1.0 / (double)m_config.targetFPS;

	// todo: can i also only set shader params once but keep updating them regardless
	// todo: also need a way to just maintain a constant buffer without constantly moving forward in the dynamic offset :))

	g_vulkanBackend->pushShaderBuffer(0, 1, VK_SHADER_STAGE_COMPUTE_BIT, particleData, particleBufferSize);

	while (m_running)
	{
		g_systemBackend->pollEvents();
		g_inputState->update();

		if (g_inputState->isPressed(KEY_ESCAPE)) {
			exit();
		}

		// ---

		double frameTime = deltaTimer.reset();
		accumulator += CalcD::min(frameTime, deltaTime);

		while (accumulator >= deltaTime) {
			accumulator -= deltaTime;
		}

		// ---

		g_vulkanBackend->beginCompute();

		g_vulkanBackend->bindShader(computeProgram);

		parameters.set("time", 1.0f);
		g_vulkanBackend->pushShaderParams(0, 0, VK_SHADER_STAGE_COMPUTE_BIT, parameters);

		g_vulkanBackend->bindShaderBuffer(0, 1);

		g_vulkanBackend->dispatchCompute(1, 1, 1);
		
		g_vulkanBackend->endCompute();

		// ---

		g_vulkanBackend->setCullMode(VK_CULL_MODE_BACK_BIT);

		g_vulkanBackend->setRenderTarget(target);
		g_vulkanBackend->beginRender();

		pushConstants.set("projMatrix", glm::perspective(70.0f, 1280.0f/720.0f, 0.1f, 10.0f));
		g_vulkanBackend->setPushConstants(pushConstants);

		g_vulkanBackend->bindShader(vertexShader);
		g_vulkanBackend->bindShader(fragmentShader);

		pass.mesh = &mesh;
		g_vulkanBackend->render(pass.build());

		g_vulkanBackend->endRender();

		// ---

		g_vulkanBackend->syncComputeWithNextRender();
		
		// ---

		g_vulkanBackend->setDepthTest(false);

		g_vulkanBackend->setRenderTarget(m_backbuffer);
		g_vulkanBackend->beginRender();

		pushConstants.set("projMatrix", glm::identity<glm::mat4>());
		g_vulkanBackend->setPushConstants(pushConstants);

		parameters.set("time", 1.0f);
		g_vulkanBackend->pushShaderParams(0, 0, VK_SHADER_STAGE_ALL_GRAPHICS, parameters);
		parameters2.set("otherData", 1.0f);
		g_vulkanBackend->pushShaderParams(1, 1, VK_SHADER_STAGE_ALL_GRAPHICS, parameters2);

		g_vulkanBackend->bindShaderBuffer(0, 2);

		g_vulkanBackend->setTexture(0, target->getAttachment(0));
		g_vulkanBackend->setSampler(0, sampler);

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

		// ---

		g_vulkanBackend->swapBuffers();
	}

	delete[] particleData;
}

void App::exit()
{
	m_running = false;

	if (m_config.onExit) {
		m_config.onExit();
	}
}
