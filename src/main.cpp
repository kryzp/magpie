#include "core/app.h"

using namespace mgp;

int main(void)
{
	Config config;
	config.windowName = "Magpie Demo";
	config.engineName = "Magpie";
	config.width = 1280;
	config.height = 720;
	config.targetFPS = 144;
	config.opacity = 1.0f;
	config.flags = 0;
	config.vsync = true;
	config.windowMode = WINDOW_MODE_WINDOWED;
	config.flags = CONFIG_FLAG_CENTRE_WINDOW_BIT | CONFIG_FLAG_RESIZABLE_BIT;

	App *app = new App(config);

	app->run();
	
	delete app;

	return 0;
}
