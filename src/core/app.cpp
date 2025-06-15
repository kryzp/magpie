#include "app.h"

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "platform/platform_core.h"
#include "graphics/graphics_core.h"

#include "math/calc.h"
#include "math/colour.h"
#include "math/timer.h"

#include "rendering/light.h"
#include "rendering/vertex_types.h"
#include "rendering/model.h"

using namespace mgp;

void App::run(const Config &config)
{
	m_config = config;

	init();

	m_running = true;

	auto obj = m_scene.createRenderObject();
	obj->model = m_modelLoader->loadModel("../../res/models/GLTF/Sponza/Sponza.gltf");
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

			m_scene.addLight(light);
		}
	}

	double accumulator = 0.0;
	const double fixedDeltaTime = 1.0 / static_cast<double>(CalcU::min(m_config.targetFPS, m_platform->getWindowRefreshRate()));

	Timer deltaTimer(m_platform);
	deltaTimer.start();

	MGP_LOG("Entering main game loop...");

	while (m_running)
	{
		m_platform->pollEvents(&m_inputSt, [&]() { exit(); }, nullptr);
		m_inputSt.update();

		if (m_inputSt.isPressed(KB_KEY_ESCAPE))
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

		CommandBuffer *cmd = m_graphics->beginPresent();
		render(cmd);
		m_graphics->present();
	}

	m_graphics->waitIdle();

	destroy();
}

void App::init()
{
	m_platform = new PlatformCore(m_config);
	m_graphics = new GraphicsCore(m_config, m_platform);
	
	m_pipelines.init(m_graphics);
	m_descriptorLayouts.init(m_graphics);
	m_imageViews.init(m_graphics);

	configure(m_config);
	
	vertex_types::initVertexTypes();

	// init imgui
	{
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();

		//ImGuiIO &io = ImGui::GetIO();

		ImGui::StyleColorsClassic();

		m_platform->initImGui();
		m_graphics->initImGui();
	}

	m_modelLoader = new ModelLoader(this);
	
	m_bindlessResources = new BindlessResources(m_graphics);

	m_textures.init(m_graphics);
	m_shaders.init(this);

	m_renderer.init(this);

	m_camera = Camera(1280.0f / 720.0f, 70.0f, 0.01f, 50.0f);
}

void App::destroy()
{
	delete m_scene.getRenderObjects()[0].model; // kys
	
	delete m_modelLoader;

	delete m_bindlessResources;

	m_renderer.destroy();

	m_shaders.destroy();
	m_textures.destroy();
	
	m_imageViews.destroy();
	m_descriptorLayouts.destroy();
	m_pipelines.destroy();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	delete m_graphics;
	delete m_platform;
}

void App::tick(float dt)
{
	if (m_inputSt.isDown(KB_KEY_F))
	{
		m_platform->setCursorVisible(false);
		m_platform->setCursorPosition(m_config.width / 2, m_config.height / 2, &m_inputSt);

		m_camera.update(&m_inputSt, m_platform, dt);
	}
	else
	{
		m_platform->setCursorVisible(true);
	}
}

void App::tickFixed(float dt)
{
}

void App::render(CommandBuffer *cmd)
{
	RenderContext context;
	context.cmd = cmd;
	context.swapchain = m_graphics->getSwapchain();
	context.scene = &m_scene;
	context.camera = &m_camera;

	m_renderer.render(context);
}

void App::configure(const Config &config)
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
	m_running = false;
}
