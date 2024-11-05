#include "app.h"

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
	config.windowMode = WINDOW_MODE_WINDOWED;

	g_app = new App(config);
	g_app->run();
	
	delete g_app;

	return 0;
}
