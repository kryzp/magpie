#ifndef APP_H_
#define APP_H_

#include "common.h"
#include "renderer.h"
#include "container/function.h"
#include "graphics/backbuffer.h"
#include "graphics/shader.h"

namespace llt
{
	enum WindowMode
	{
		WINDOW_MODE_WINDOWED	= 0 << 0,
		WINDOW_MODE_BORDERLESS	= 1 << 0,
		WINDOW_MODE_FULLSCREEN	= 1 << 1,

		WINDOW_MODE_BORDERLESS_FULLSCREEN = WINDOW_MODE_BORDERLESS | WINDOW_MODE_FULLSCREEN
	};

	struct Config
	{
		enum ConfigFlag
		{
			FLAG_NONE			    = 0 << 0,
			FLAG_RESIZABLE          = 1 << 0,
			FLAG_VSYNC              = 1 << 1,
			FLAG_CURSOR_INVISIBLE   = 1 << 2,
			FLAG_CENTRE_WINDOW      = 1 << 3,
			FLAG_HIGH_PIXEL_DENSITY = 1 << 4
		};

		const char* name = nullptr;
		unsigned width = 1280;
		unsigned height = 720;
		unsigned targetFPS = 60;
		float opacity = 1.0f;
		int flags = 0;
		bool vsync = false;

		WindowMode windowMode = WINDOW_MODE_WINDOWED;

		Function<void(void)> onInit = nullptr;
		Function<void(void)> onExit = nullptr;
		Function<void(void)> onDestroy = nullptr;

		constexpr bool hasFlag(ConfigFlag flag) const { return flags & flag; }
	};

	class App
	{
	public:
		App(const Config& config);
		~App();

		void run();
		void exit();

	private:
		Config m_config;
		bool m_running;

		Camera m_camera;
		Renderer m_renderer;
	};

	extern App* g_app;
}

#endif // APP_H_
