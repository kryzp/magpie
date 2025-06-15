#pragma once

namespace mgp
{	
	enum WindowMode
	{
		WINDOW_MODE_WINDOWED,
		WINDOW_MODE_BORDERLESS,
		WINDOW_MODE_FULLSCREEN,
		WINDOW_MODE_BORDERLESS_FULLSCREEN
	};

	enum ConfigFlag
	{
		CONFIG_FLAG_NONE					= 0 << 0,
		CONFIG_FLAG_RESIZABLE_BIT			= 1 << 0,
		CONFIG_FLAG_VSYNC_BIT				= 1 << 1,
		CONFIG_FLAG_CURSOR_INVISIBLE_BIT	= 1 << 2,
		CONFIG_FLAG_CENTRE_WINDOW_BIT		= 1 << 3,
		CONFIG_FLAG_HIGH_PIXEL_DENSITY_BIT	= 1 << 4,
		CONFIG_FLAG_LOCK_CURSOR_BIT			= 1 << 5
	};

	struct Config
	{
		struct Version
		{
			unsigned variant = 0;
			unsigned major = 1;
			unsigned minor = 0;
			unsigned patch = 0;
		};

		const char *windowName = nullptr;
		const char *engineName = nullptr;

		Version appVersion;
		Version engineVersion;

		unsigned width = 1280;
		unsigned height = 720;
		unsigned targetFPS = 60;
		float opacity = 1.0f;
		int flags = 0;
		bool vsync = false;

		WindowMode windowMode = WINDOW_MODE_WINDOWED;

		constexpr bool hasFlag(ConfigFlag flag) const { return flags & flag; }
	};
}
