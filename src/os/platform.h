#ifndef OS_PLATFORM_H
#define OS_PLATFORM_H

#include "core/types.h"
#include "core/vec2.h"

#include "container/string.h"

#include <vulkan/vulkan.h>

struct SDL_Window;
struct SDL_Cursor;

#define OS_APPLICATION_NAME         "darkwreath"
#define OS_ENGINE_NAME              "magpie"

#define OS_APP_VERSION_MAJOR         0
#define OS_APP_VERSION_MINOR         1
#define OS_APP_VERSION_PATCH         0

#define OS_ENGINE_VERSION_MAJOR      0
#define OS_ENGINE_VERSION_MINOR      1
#define OS_ENGINE_VERSION_PATCH      0

namespace os {

class Window {
public:
	Window(const char *title, unsigned width, unsigned height);
	~Window();

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	void SetTitle(const String &title);

	void GetSize(unsigned *w, unsigned *h) const;
	void GetSizeInPixels(unsigned *pw, unsigned *ph) const;

	void SetSize(unsigned w, unsigned h);

	void SetFullscreen(bool fullscreen);
	void SetBorderless(bool borderless);
	void SetOpacity(float opacity);

	void SetMousePosition(float x, float y);
	void SetMouseVisible(bool visible);
	[[nodiscard]] bool IsMouseVisible() const;
	void SetMouseLocked(bool locked);
	[[nodiscard]] bool IsMouseLocked() const;

	bool SetCursorImage(const u8 *rgba, unsigned width, unsigned height, int hot_x, int hot_y);

	[[nodiscard]] Vec2 ScreenToNDC(const Vec2 &window_position) const;

	bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface) const;
	void DestroyVulkanSurface(VkInstance instance, VkSurfaceKHR surface) const;

	[[nodiscard]] SDL_Window *GetSDLWindow() const
	{
		return sdl_window;
	}

	[[nodiscard]] bool IsValid() const
	{
		return sdl_window != nullptr;
	}

private:
	SDL_Window *sdl_window;
	SDL_Cursor *cursor;
};

u64 GetPageSize();
unsigned GetNumCores();

u64 GetTicks();
void SleepForMs(unsigned ms);
u64 GetPerformanceCounter();
u64 GetPerformanceFrequency();

const char *const *GetVulkanInstanceExtensions(unsigned *count);

void OpenInExplorer(const String &path);

}

#endif // OS_PLATFORM_H
