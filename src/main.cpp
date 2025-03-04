#include "core/app.h"

using namespace llt;

int main(void)
{
	Config config;
	config.name = "Lilythorn";
	config.width = 1280;
	config.height = 720;
	config.targetFPS = 144;
	config.opacity = 1.0f;
	config.flags = 0;
	config.vsync = true;
	config.windowMode = WINDOW_MODE_WINDOWED;
	config.flags = Config::FLAG_CENTRE_WINDOW;

	g_app = new App(config);
	g_app->run();
	
	delete g_app;

	return 0;
}
