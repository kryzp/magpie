#include "app.h"

#include "platform.h"
#include "debug_ui.h"
#include "profiler.h"

#include "vulkan/core.h"

#include "rendering/camera.h"
#include "rendering/material_system.h"

#include "input/input.h"

#include "math/timer.h"
#include "math/calc.h"
#include "math/colour.h"

llt::App *llt::g_app = nullptr;

using namespace llt;

App::App(const Config &config)
	: m_config(config)
	, m_running(false)
	, m_camera(config.width, config.height, 75.0f, 0.01f, 100.0f)
	, m_renderer()
	, m_frameCount(0)
{
	g_platform = new Platform(config);
	g_vkCore = new VulkanCore(config);

	g_inputState = new Input();

	Swapchain *swapchain = g_vkCore->createSwapchain();
	swapchain->setClearColours(Colour::black());

	g_platform->initImGui();

	g_platform->setWindowName(m_config.name);
	g_platform->setWindowSize({ m_config.width, m_config.height });
	g_platform->setWindowMode(m_config.windowMode);
	g_platform->setWindowOpacity(m_config.opacity);
	g_platform->setCursorVisible(!m_config.hasFlag(Config::FLAG_CURSOR_INVISIBLE_BIT));
	g_platform->toggleWindowResizable(m_config.hasFlag(Config::FLAG_RESIZABLE_BIT));
	g_platform->lockCursor(m_config.hasFlag(Config::FLAG_LOCK_CURSOR_BIT));

	if (m_config.hasFlag(Config::FLAG_CENTRE_WINDOW_BIT))
	{
		glm::ivec2 screenSize = g_platform->getScreenSize();

		g_platform->setWindowPosition({
			(int)((screenSize.x - m_config.width) * 0.5f),
			(int)((screenSize.y - m_config.height) * 0.5f)
		});
	}

	g_profiler = new Profiler(100);

	m_running = true;

	m_camera.position = glm::vec3(0.0f, 2.75f, 3.5f);
	m_camera.setPitch(-glm::radians(30.0f));
	m_camera.update(0.0f);

	m_renderer.init();

	dbgui::init();

	if (m_config.onInit)
		m_config.onInit();
}

App::~App()
{
	if (m_config.onDestroy)
		m_config.onDestroy();

	m_renderer.cleanUp();

	delete g_profiler;
	delete g_inputState;

	delete g_vkCore;
	delete g_platform;
}

void App::run()
{
	double accumulator = 0.0;
	const double fixedDeltaTime = 1.0 / static_cast<double>(CalcU::min(m_config.targetFPS, g_platform->getWindowRefreshRate()));
	
	Timer deltaTimer;
	deltaTimer.start();

	while (m_running)
	{
		g_platform->pollEvents();
		g_inputState->update();

		if (g_inputState->isPressed(KB_KEY_ESCAPE)) {
			exit();
		}

		g_platform->imGuiNewFrame();

		double deltaTime = deltaTimer.reset();

		accumulator += CalcF::min(deltaTime, fixedDeltaTime);

		while (accumulator >= fixedDeltaTime)
		{
			if (g_inputState->isDown(KB_KEY_F))
			{
				g_platform->setCursorVisible(false);
				g_platform->setCursorPosition(m_config.width / 2, m_config.height / 2);

				m_camera.update(fixedDeltaTime);
			}
			else
			{
				g_platform->setCursorVisible(true);
			}

			accumulator -= fixedDeltaTime;
		}

		dbgui::update();

		m_renderer.render(m_camera, deltaTime);

		m_frameCount++;
	}
}

void App::exit()
{
	m_running = false;

	if (m_config.onExit)
		m_config.onExit();
}
