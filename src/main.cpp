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
	config.vsync = true;
	config.windowMode = WINDOW_MODE_WINDOWED;
	config.flags = Config::FLAG_CENTRE_WINDOW | Config::FLAG_CURSOR_INVISIBLE;

	g_app = new App(config);
	g_app->run();
	
	delete g_app;

	return 0;
}

// dump
/*
void sortBoundOffsets(Vector<Pair<int, uint32_t>>& offsets, int lo, int hi)
{
if (lo >= hi || lo < 0) {
return;
}

int pivot = offsets[hi].first;
int i = lo - 1;

for (int j = lo; j < hi; j++)
{
if (offsets[j].first <= pivot)
{
i++;

auto tmp = offsets[i];
offsets[i] = offsets[j];
offsets[j] = tmp;
}
}

auto tmp = offsets[i + 1];
offsets[i + 1] = offsets[hi];
offsets[hi] = tmp;

int partition = i + 1;

sortBoundOffsets(offsets, lo, partition - 1);
sortBoundOffsets(offsets, partition + 1, hi);
}
*/
