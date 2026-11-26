#ifndef OS_APPLICATION_H
#define OS_APPLICATION_H

#include "core/types.h"
#include "core/log.h"
#include "os/input.h"

namespace os {
class Window;

class IApplication {
public:
	virtual ~IApplication() = default;
	virtual void Init(Window *window) = 0;
	virtual void Destroy() = 0;
	virtual bool Tick(const InputState &input) = 0;
};

struct RunConfig {

	const char *window_title = "unnamed";

	unsigned window_width = 1280;
	unsigned window_height = 720;

	bool enable_imgui = true;

	const char *log_sink = nullptr;
};

int Run(IApplication *app, const RunConfig &config);
void RequestQuit();

LogChannel GetLogChannel();

}

#endif // OS_APPLICATION_H
