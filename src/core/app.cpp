#include "app.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "platform.h"

#include "vulkan/core.h"
#include "vulkan/toolbox.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/swapchain.h"
#include "vulkan/sync.h"
#include "vulkan/shader.h"
#include "vulkan/image.h"
#include "vulkan/sampler.h"
#include "vulkan/vertex_format.h"

#include "rendering/model_loader.h"
#include "rendering/model.h"

#include "input/input.h"

#include "math/timer.h"
#include "math/calc.h"
#include "math/colour.h"

using namespace mgp;

App::App(const Config &config)
	: m_textures()
	, m_shaders()
	, m_renderer()
	, m_scene()
	, m_config(config)
	, m_vulkanCore(nullptr)
	, m_platform(nullptr)
	, m_inputSt(nullptr)
	, m_running(false)
	, m_camera((float)config.width / (float)config.height, 75.0f, 0.01f, 100.0f)
{
	m_platform = new Platform(config);
	m_vulkanCore = new VulkanCore(config, m_platform);
	
	applyConfig(config);

	m_inputSt = new InputState();

	vtx::initVertexTypes();

	m_camera.position = glm::vec3(0.0f, 2.75f, 3.5f);
	m_camera.setPitch(-glm::radians(30.0f));
	m_camera.update(m_inputSt, m_platform, 0.0f);

	m_textures.init(m_vulkanCore);
	m_shaders.init(m_vulkanCore);

	m_platform->onExit = [this]() { exit(); };

	m_running = true;
}

App::~App()
{
	delete m_scene.getRenderList()[0]->getParent(); // crap

	m_renderer.destroy();

	m_textures.destroy();
	m_shaders.destroy();

	delete m_inputSt;

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	delete m_vulkanCore;
	delete m_platform;
}

void App::run()
{
	InFlightSync inFlightSync(m_vulkanCore);

	// init imgui
	{
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();

		//ImGuiIO &io = ImGui::GetIO();

		ImGui::StyleColorsClassic();

		m_platform->initImGui();
		m_vulkanCore->initImGui();
	}

	m_platform->onWindowResize = [&inFlightSync](int w, int h) -> void {
		MGP_LOG("Detected window resize!");
		inFlightSync.onWindowResize(w, h);
	};

	double accumulator = 0.0;
	const double fixedDeltaTime = 1.0 / static_cast<double>(CalcU::min(m_config.targetFPS, m_platform->getWindowRefreshRate()));

	Timer deltaTimer(m_platform);
	deltaTimer.start();
	
	m_renderer.init(this, inFlightSync.getSwapchain());

	ModelLoader modelLoader(m_vulkanCore, this);
	Model *model = modelLoader.loadModel("../../res/models/GLTF/Sponza/Sponza.gltf");

	auto obj = m_scene.createRenderObject();
	obj->model = model;
	obj->model->setOwner(obj);
	obj->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	obj->transform.setRotation(0.0f, { 0.0f, 1.0f, 0.0f });
	obj->transform.setScale({ 3.0f, 3.0f, 3.0f });
	obj->transform.setOrigin({ 0.0f, 0.0f, 0.0f });

	Colour colours[] = {
		Colour::white(),
		Colour::red(),
		Colour::green(),
		Colour::blue(),
		Colour::cyan(),
		Colour::magenta(),
		Colour::yellow()
	};

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			Light light;
			light.setType(Light::TYPE_POINT);
			light.setIntensity(3.0f);
			light.setFalloff(1.0f);
			light.setPosition({ i*4.0f, 1.0f, j*4.0f - 2.0f });
			light.setColour(colours[(j*4 + i) % MGP_ARRAY_LENGTH(colours)]);
			light.toggleShadows(false);

			m_scene.addLight(light);
		}
	}

	MGP_LOG("Entering main game loop...");

	while (m_running)
	{
		m_platform->pollEvents(m_inputSt);
		m_inputSt->update();

		if (m_inputSt->isPressed(KB_KEY_ESCAPE))
			exit();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		double deltaTime = deltaTimer.reset();

		tick(deltaTime);

		accumulator += CalcD::min(deltaTime, fixedDeltaTime);

		while (accumulator >= fixedDeltaTime)
		{
			tickFixed(fixedDeltaTime);

			accumulator -= fixedDeltaTime;
		}

		inFlightSync.begin();
		render(inFlightSync.getSwapchain());
		inFlightSync.present();

		m_vulkanCore->nextFrame();
	}

	m_vulkanCore->deviceWaitIdle();

	inFlightSync.destroy();
}

void App::tick(float dt)
{
	if (m_inputSt->isDown(KB_KEY_F))
	{
		m_platform->setCursorVisible(false);
		m_platform->setCursorPosition(m_config.width / 2, m_config.height / 2, m_inputSt);

		m_camera.update(m_inputSt, m_platform, dt);
	}
	else
	{
		m_platform->setCursorVisible(true);
	}
}

void App::tickFixed(float dt)
{
}

void App::render(Swapchain *swapchain)
{
	m_renderer.render(m_scene, m_camera, swapchain);
}

void App::applyConfig(const Config &config)
{
	m_platform->setWindowName(config.windowName);
	m_platform->setWindowSize({ config.width, config.height });
	m_platform->setWindowMode(config.windowMode);
	m_platform->setWindowOpacity(config.opacity);
	m_platform->setCursorVisible(!config.hasFlag(CONFIG_FLAG_CURSOR_INVISIBLE_BIT));
	m_platform->toggleWindowResizable(config.hasFlag(CONFIG_FLAG_RESIZABLE_BIT));
	m_platform->lockCursor(config.hasFlag(CONFIG_FLAG_LOCK_CURSOR_BIT));

	if (config.hasFlag(CONFIG_FLAG_CENTRE_WINDOW_BIT))
	{
		glm::ivec2 screenSize = m_platform->getScreenSize();

		m_platform->setWindowPosition({
			(int)((screenSize.x - config.width) * 0.5f),
			(int)((screenSize.y - config.height) * 0.5f)
		});
	}
}

void App::exit()
{
	MGP_LOG("Detected window close event, quitting...");

	m_running = false;
}
