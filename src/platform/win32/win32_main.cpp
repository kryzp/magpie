#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

// minwindef.h wtf???
// What stupid programmer decided to make these defines global ffs.
#undef near
#undef far

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "platform/platform.h"
#include "math/vec2.h"
#include "core/types.h"

#include "app.h"

static SDL_Window *win32_sdl_window = nullptr;
static Platform win32_platform = {};

static bool win32_create_vulkan_surface(void *instance, void *surface_pointer)
{
	return SDL_Vulkan_CreateSurface(win32_sdl_window, (VkInstance)instance, nullptr, (VkSurfaceKHR *)surface_pointer);
}

static void win32_destroy_vulkan_surface(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface((VkInstance)instance, (VkSurfaceKHR)surface, nullptr);
}

static void win32_reconnect_all_gamepads()
{
	// TODO
}

static void win32_close_all_gamepads()
{
	// TODO
}

static void win32_set_window_size(u32 width, u32 height)
{
	SDL_SetWindowSize(win32_sdl_window, width, height);

	SDL_GetWindowSizeInPixels(win32_sdl_window,
		&win32_platform.window_pixel_width,
		&win32_platform.window_pixel_height
	);
}

static void win32_set_window_fullscreen(bool b)
{
	SDL_SetWindowFullscreen(win32_sdl_window, b);
}

static void win32_set_window_borderless(bool b)
{
	SDL_SetWindowBordered(win32_sdl_window, !b);
}

static void win32_set_mouse_position(u32 x, u32 y)
{
	win32_platform.mouse_position = Vec2(x, y);

	SDL_WarpMouseInWindow(win32_sdl_window, x, y);

	SDL_GetGlobalMouseState(
		&win32_platform.mouse_screen_position.x,
		&win32_platform.mouse_screen_position.y
	);
}

static bool win32_file_delete(const char *path)
{
	return std::filesystem::remove(path);
}

static bool win32_file_exists(const char *path)
{
	return std::filesystem::exists(path);
}

static bool win32_dir_create(const char *path)
{
	return std::filesystem::create_directory(path);
}

static bool win32_dir_delete(const char *path)
{
	return std::filesystem::remove_all(path) > 0;
}

static bool win32_dir_exists(const char *path)
{
	return std::filesystem::is_directory(path);
}

static void *win32_stream_from_file(const char *path, const char *mode)
{
	return SDL_IOFromFile(path, mode);
}

static void *win32_stream_from_memory(void *data, u64 size)
{
	return SDL_IOFromMem(data, size);
}

static void *win32_stream_from_const_memory(const void *data, u64 size)
{
	return SDL_IOFromConstMem(data, size);
}

static s64 win32_stream_read(void *stream, void *dst, u64 size)
{
	return SDL_ReadIO((SDL_IOStream *)stream, dst, size);
}

static s64 win32_stream_write(void *stream, const void *src, u64 size)
{
	return SDL_WriteIO((SDL_IOStream *)stream, src, size);
}

static s64 win32_stream_seek(void *stream, s64 offset)
{
	return SDL_SeekIO((SDL_IOStream *)stream, offset, SDL_IO_SEEK_SET);
}

static s64 win32_stream_size(void *stream)
{
	return SDL_GetIOSize((SDL_IOStream *)stream);
}

static s64 win32_stream_position(void *stream)
{
	return SDL_TellIO((SDL_IOStream *)stream);
}

static bool win32_stream_close(void *stream)
{
	return SDL_CloseIO((SDL_IOStream *)stream);
}

static void win32_open_in_explorer(const char *path)
{
	ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
}

static void init_platform()
{
	win32_platform.window_title = DEFAULT_WINDOW_TITLE;

	win32_platform.window_width = DEFAULT_WINDOW_WIDTH;
	win32_platform.window_height = DEFAULT_WINDOW_HEIGHT;

	SDL_GetWindowSizeInPixels(win32_sdl_window,
		&win32_platform.window_pixel_width,
		&win32_platform.window_pixel_height
	);

	win32_platform.window_opacity = 1.f;

	win32_platform.fullscreen = false;
	win32_platform.borderless = false;

	win32_platform.target_fps = 120;
	win32_platform.current_time = 0.f;

	win32_platform.cursor_visible = true;
	win32_platform.cursor_locked = false;

	win32_platform.set_window_size = win32_set_window_size;
	win32_platform.set_window_fullscreen = win32_set_window_fullscreen;
	win32_platform.set_window_borderless = win32_set_window_borderless;

	win32_platform.set_mouse_position = win32_set_mouse_position;

	win32_platform.get_ticks = SDL_GetTicks;
	win32_platform.get_performance_counter = SDL_GetPerformanceCounter;
	win32_platform.get_performance_frequency = SDL_GetPerformanceFrequency;

	win32_platform.file_delete = win32_file_delete;
	win32_platform.file_exists = win32_file_exists;

	win32_platform.dir_create = win32_dir_create;
	win32_platform.dir_delete = win32_dir_delete;
	win32_platform.dir_exists = win32_dir_exists;

	win32_platform.stream_from_file = win32_stream_from_file;
	win32_platform.stream_from_memory = win32_stream_from_memory;
	win32_platform.stream_from_const_memory = win32_stream_from_const_memory;
	win32_platform.stream_read = win32_stream_read;
	win32_platform.stream_write = win32_stream_write;
	win32_platform.stream_seek = win32_stream_seek;
	win32_platform.stream_size = win32_stream_size;
	win32_platform.stream_position = win32_stream_position;
	win32_platform.stream_close = win32_stream_close;

	win32_platform.open_in_explorer = win32_open_in_explorer;

	win32_platform.create_vulkan_surface = win32_create_vulkan_surface;
	win32_platform.destroy_vulkan_surface = win32_destroy_vulkan_surface;
	win32_platform.get_vulkan_instance_extensions = SDL_Vulkan_GetInstanceExtensions;
}

int main(int argc, char **argv)
{
	SDL_InitFlags init_flags =
		SDL_INIT_VIDEO |
		SDL_INIT_AUDIO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMEPAD |
		SDL_INIT_HAPTIC |
		SDL_INIT_EVENTS |
		SDL_INIT_SENSOR |
		SDL_INIT_CAMERA;

	if (!SDL_Init(init_flags)) {
		debug_log_crash("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_WindowFlags window_flags =
		SDL_WINDOW_VULKAN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY;

	win32_sdl_window = SDL_CreateWindow(
		DEFAULT_WINDOW_TITLE,
		DEFAULT_WINDOW_WIDTH,
		DEFAULT_WINDOW_HEIGHT,
		window_flags
	);

	if (!win32_sdl_window) {
		debug_log_crash("Failed to create SDL window: %s", SDL_GetError());
		return -1;
	}

	init_platform();

	Platform prev_st = win32_platform;

	App app(win32_platform);
	app.init();

	debug_log("Entering main loop...");

	bool running = true;

	while (running) {
		SDL_Event ev = {0};

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_EVENT_QUIT:
				running = false;
				break;

			case SDL_EVENT_WINDOW_RESIZED:
				break;

			case SDL_EVENT_KEY_DOWN:
				win32_platform.kb_down[ev.key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				win32_platform.kb_down[ev.key.scancode] = false;
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				win32_platform.mb_down[ev.button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				win32_platform.mb_down[ev.button.button] = false;
				break;

			case SDL_EVENT_MOUSE_MOTION: {
				float spx = 0.f;
				float spy = 0.f;

				SDL_GetGlobalMouseState(&spx, &spy);

				win32_platform.mouse_position = Vec2(ev.motion.x, ev.motion.y);
				win32_platform.mouse_delta = Vec2(ev.motion.xrel, ev.motion.yrel);
				win32_platform.mouse_screen_position = Vec2(spx, spy);
			} break;

			case SDL_EVENT_MOUSE_WHEEL:
				win32_platform.mouse_wheel = Vec2(ev.wheel.x, ev.wheel.y);
				break;
			}
		}

		for (int i = 0; i < KEYBOARD_KEY_MAX_ENUM; i++) {
			win32_platform.kb_pressed [i] =  win32_platform.kb_down[i] && !prev_st.kb_down[i];
			win32_platform.kb_released[i] = !win32_platform.kb_down[i] &&  prev_st.kb_down[i];
		}

		for (int i = 0; i < MBUTTON_MAX_ENUM; i++) {
			win32_platform.mb_pressed [i] =  win32_platform.mb_down[i] && !prev_st.mb_down[i];
			win32_platform.mb_released[i] = !win32_platform.mb_down[i] &&  prev_st.mb_down[i];
		}

		for (int i = 0; i < MAX_GAMEPADS; i++) {
			GamepadState *st = &win32_platform.gamepads[i];
			GamepadState *p_st = &prev_st.gamepads[i];

			for (int j = 0; j < GAMEPAD_BUTTON_MAX_ENUM; j++) {
				st->pressed [i] =  st->down[i] && !p_st->down[i];
				st->released[i] = !st->down[i] &&  p_st->down[i];
			}
		}

		if (app.tick())
			break;

		prev_st = win32_platform;
	}

	app.destroy();

	win32_close_all_gamepads();

	SDL_DestroyWindow(win32_sdl_window);
	SDL_Quit();

	return 0;
}
