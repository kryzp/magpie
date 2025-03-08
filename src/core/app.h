#ifndef APP_H_
#define APP_H_

#include "common.h"
#include "container/function.h"
#include "vulkan/backbuffer.h"
#include "vulkan/shader.h"
#include "rendering/renderer.h"
#include "rendering/camera.h"

namespace llt
{
	enum WindowMode
	{
		WINDOW_MODE_WINDOWED_BIT	= 0 << 0,
		WINDOW_MODE_BORDERLESS_BIT	= 1 << 0,
		WINDOW_MODE_FULLSCREEN_BIT	= 1 << 1,

//		WINDOW_MODE_BORDERLESS_FULLSCREEN_BIT = WINDOW_MODE_BORDERLESS_BIT | WINDOW_MODE_FULLSCREEN_BIT
	};

	struct Config
	{
		enum ConfigFlag
		{
			FLAG_NONE_BIT			    = 0 << 0,
			FLAG_RESIZABLE_BIT          = 1 << 0,
			FLAG_VSYNC_BIT              = 1 << 1,
			FLAG_CURSOR_INVISIBLE_BIT   = 1 << 2,
			FLAG_CENTRE_WINDOW_BIT      = 1 << 3,
			FLAG_HIGH_PIXEL_DENSITY_BIT = 1 << 4,
			FLAG_LOCK_CURSOR_BIT		= 1 << 5
		};

		const char *name = nullptr;
		unsigned width = 1280;
		unsigned height = 720;
		unsigned targetFPS = 60;
		float opacity = 1.0f;
		int flags = 0;
		bool vsync = false;

		WindowMode windowMode = WINDOW_MODE_WINDOWED_BIT;

		Function<void(void)> onInit = nullptr;
		Function<void(void)> onExit = nullptr;
		Function<void(void)> onDestroy = nullptr;

		constexpr bool hasFlag(ConfigFlag flag) const { return flags & flag; }
	};

	class App
	{
	public:
		App(const Config &config);
		~App();

		void run();
		void exit();

	private:
		Config m_config;
		bool m_running;
		int m_frameCount;

		Camera m_camera;
		Renderer m_renderer;
	};

	extern App *g_app;
}

#endif // APP_H_
