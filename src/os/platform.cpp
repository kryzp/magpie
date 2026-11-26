#include "os/platform.h"
#include "core/log.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#if defined(__linux__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sched.h>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

static u64 page_size = 0;
static unsigned num_cores = 0;

static void QuerySystemInfo()
{
#if defined(_WIN32)

	SYSTEM_INFO info = {};
	GetSystemInfo(&info);

	page_size = info.dwPageSize;
	num_cores = info.dwNumberOfProcessors;

#else

	long page = sysconf(_SC_PAGESIZE);
	page_size = (page > 0) ? static_cast<u64>(page) : 4096;

	unsigned cores = 0;

#if defined(__linux__)
	cpu_set_t set;
	CPU_ZERO(&set);

	if (sched_getaffinity(0, sizeof(set), &set) == 0)
		cores = static_cast<unsigned>(CPU_COUNT(&set));
#endif

	if (cores == 0) {
		long n = sysconf(_SC_NPROCESSORS_ONLN);
		cores = (n > 0) ? static_cast<unsigned>(n) : 1;
	}

	num_cores = cores;

#endif
}

u64 os::GetPageSize()
{
	if (page_size == 0)
		QuerySystemInfo();

	return page_size;
}

unsigned os::GetNumCores()
{
	if (num_cores == 0)
		QuerySystemInfo();

	return num_cores;
}

u64 os::GetTicks()
{
	return SDL_GetTicks();
}

void os::SleepForMs(unsigned ms)
{
	SDL_Delay(ms);
}

u64 os::GetPerformanceCounter()
{
	return SDL_GetPerformanceCounter();
}

u64 os::GetPerformanceFrequency()
{
	return SDL_GetPerformanceFrequency();
}

const char *const *os::GetVulkanInstanceExtensions(unsigned *count)
{
	Uint32 sdl_count = 0;
	const char *const *names = SDL_Vulkan_GetInstanceExtensions(&sdl_count);
	*count = sdl_count;
	return names;
}

void os::OpenInExplorer(const String &path)
{
	// TODO: not finished

	DebugPrintW("Not implemented! But here we should open the following path in the system file explorer: \"%s\"", path.c_str());
}

os::Window::Window(const char *title, unsigned width, unsigned height)
	: sdl_window(nullptr)
	, cursor(nullptr)
{
	SDL_WindowFlags window_flags =
		SDL_WINDOW_VULKAN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY;

	sdl_window = SDL_CreateWindow(title, static_cast<int>(width), static_cast<int>(height), window_flags);

	if (!sdl_window)
		fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
}

os::Window::~Window()
{
	if (cursor) {
		SDL_SetCursor(SDL_GetDefaultCursor());
		SDL_DestroyCursor(cursor);
	}

	if (sdl_window)
		SDL_DestroyWindow(sdl_window);
}

void os::Window::SetTitle(const String &title)
{
	SDL_SetWindowTitle(sdl_window, title.c_str());
}

void os::Window::GetSize(unsigned *w, unsigned *h) const
{
	int width = 0;
	int height = 0;

	SDL_GetWindowSize(sdl_window, &width, &height);

	AssertTrue(width >= 0 && height >= 0);

	if (w) *w = width;
	if (h) *h = height;
}

void os::Window::GetSizeInPixels(unsigned *pw, unsigned *ph) const
{
	int pixel_width = 0;
	int pixel_height = 0;

	SDL_GetWindowSizeInPixels(sdl_window, &pixel_width, &pixel_height);

	AssertTrue(pixel_width >= 0 && pixel_height >= 0);

	if (pw) *pw = pixel_width;
	if (ph) *ph = pixel_height;
}

void os::Window::SetSize(unsigned w, unsigned h)
{
	SDL_SetWindowSize(sdl_window, w, h);
}

void os::Window::SetFullscreen(bool fullscreen)
{
	SDL_SetWindowFullscreen(sdl_window, fullscreen);
}

void os::Window::SetBorderless(bool borderless)
{
	SDL_SetWindowBordered(sdl_window, !borderless);
}

void os::Window::SetOpacity(float opacity)
{
	SDL_SetWindowOpacity(sdl_window, opacity);
}

void os::Window::SetMousePosition(float x, float y)
{
	SDL_WarpMouseInWindow(sdl_window, x, y);
}

void os::Window::SetMouseVisible(bool visible)
{
	if (visible)
		SDL_ShowCursor();
	else
		SDL_HideCursor();
}

bool os::Window::IsMouseVisible() const
{
	return SDL_CursorVisible();
}

void os::Window::SetMouseLocked(bool locked)
{
	SDL_SetWindowRelativeMouseMode(sdl_window, locked);
}

bool os::Window::IsMouseLocked() const
{
	return SDL_GetWindowRelativeMouseMode(sdl_window);
}

bool os::Window::SetCursorImage(u8 *rgba, unsigned width, unsigned height, int hot_x, int hot_y)
{
	SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(width), static_cast<int>(height), SDL_PIXELFORMAT_RGBA32, rgba, static_cast<int>(width) * 4);

	if (!surface)
		return false;

	SDL_Cursor *created = SDL_CreateColorCursor(surface, hot_x, hot_y);
	SDL_DestroySurface(surface);

	if (!created)
		return false;

	SDL_SetCursor(created);

	if (cursor)
		SDL_DestroyCursor(cursor);

	cursor = created;
	return true;
}

Vec2 os::Window::ScreenToNDC(const Vec2 &window_position) const
{
	unsigned window_width = 0;
	unsigned window_height = 0;

	GetSize(&window_width, &window_height);

	float x = (2.f * window_position.x) / static_cast<float>(window_width) - 1.f;
	float y = (2.f * window_position.y) / static_cast<float>(window_height) - 1.f;

	y *= -1.f;

	return Vec2(x, y);
}

bool os::Window::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *surface) const
{
	return SDL_Vulkan_CreateSurface(sdl_window, instance, nullptr, surface);
}

void os::Window::DestroyVulkanSurface(VkInstance instance, VkSurfaceKHR surface) const
{
	SDL_Vulkan_DestroySurface(instance, surface, nullptr);
}
