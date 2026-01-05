#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <filesystem>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "platform/platform.h"
#include "math/vec2.h"
#include "core/types.h"

#include "app.h"

static SDL_Window *macos_sdl_window = nullptr;
static Platform macos_platform = {};

static bool macos_create_vulkan_surface(void *instance, void *surface_pointer)
{
	return SDL_Vulkan_CreateSurface(macos_sdl_window, (VkInstance)instance, nullptr, (VkSurfaceKHR *)surface_pointer);
}

static void macos_destroy_vulkan_surface(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface((VkInstance)instance, (VkSurfaceKHR)surface, nullptr);
}

static void macos_reconnect_all_gamepads()
{
	// TODO(kp)
}

static void macos_close_all_gamepads()
{
	// TODO(kp)
}

static void macos_set_window_size(u32 width, u32 height)
{
	SDL_SetWindowSize(macos_sdl_window, width, height);

	SDL_GetWindowSizeInPixels(macos_sdl_window,
		&macos_platform.window_pixel_width,
		&macos_platform.window_pixel_height
	);
}

static void macos_set_window_fullscreen(bool b)
{
	SDL_SetWindowFullscreen(macos_sdl_window, b);
}

static void macos_set_window_borderless(bool b)
{
	SDL_SetWindowBordered(macos_sdl_window, !b);
}

static void macos_set_mouse_position(u32 x, u32 y)
{
	macos_platform.mouse_position = Vec2(x, y);

	SDL_WarpMouseInWindow(macos_sdl_window, x, y);

	SDL_GetGlobalMouseState(
		&macos_platform.mouse_screen_position.x,
		&macos_platform.mouse_screen_position.y
	);
}

static bool macos_file_delete(const char *path)
{
	return std::filesystem::remove(path);
}

static bool macos_file_exists(const char *path)
{
	return std::filesystem::exists(path);
}

static bool macos_dir_create(const char *path)
{
	return std::filesystem::create_directory(path);
}

static bool macos_dir_delete(const char *path)
{
	return std::filesystem::remove_all(path) > 0;
}

static bool macos_dir_exists(const char *path)
{
	return std::filesystem::is_directory(path);
}

static void *macos_stream_from_file(const char *path, const char *mode)
{
	return SDL_IOFromFile(path, mode);
}

static void *macos_stream_from_memory(void *data, u64 size)
{
	return SDL_IOFromMem(data, size);
}

static void *macos_stream_from_const_memory(const void *data, u64 size)
{
	return SDL_IOFromConstMem(data, size);
}

static s64 macos_stream_read(void *stream, void *dst, u64 size)
{
	return SDL_ReadIO((SDL_IOStream *)stream, dst, size);
}

static s64 macos_stream_write(void *stream, const void *src, u64 size)
{
	return SDL_WriteIO((SDL_IOStream *)stream, src, size);
}

static s64 macos_stream_seek(void *stream, s64 offset)
{
	return SDL_SeekIO((SDL_IOStream *)stream, offset, SDL_IO_SEEK_SET);
}

static s64 macos_stream_size(void *stream)
{
	return SDL_GetIOSize((SDL_IOStream *)stream);
}

static s64 macos_stream_position(void *stream)
{
	return SDL_TellIO((SDL_IOStream *)stream);
}

static bool macos_stream_close(void *stream)
{
	return SDL_CloseIO((SDL_IOStream *)stream);
}

static void macos_open_in_explorer(const char *path)
{
	char call[512] = {};
	snprintf(call, sizeof(call), "open \"%s\"", path);
	system(call);
}

static void init_platform()
{
	macos_platform.window_title = DEFAULT_WINDOW_TITLE;

	macos_platform.window_width = DEFAULT_WINDOW_WIDTH;
	macos_platform.window_height = DEFAULT_WINDOW_HEIGHT;

	SDL_GetWindowSizeInPixels(macos_sdl_window,
		&macos_platform.window_pixel_width,
		&macos_platform.window_pixel_height
	);

	macos_platform.window_opacity = 1.f;

	macos_platform.fullscreen = false;
	macos_platform.borderless = false;

	macos_platform.target_fps = 120;
	macos_platform.current_time = 0.f;

	macos_platform.cursor_visible = true;
	macos_platform.cursor_locked = false;

	macos_platform.set_window_size = macos_set_window_size;
	macos_platform.set_window_fullscreen = macos_set_window_fullscreen;
	macos_platform.set_window_borderless = macos_set_window_borderless;

	macos_platform.set_mouse_position = macos_set_mouse_position;

	macos_platform.get_ticks = SDL_GetTicks;
	macos_platform.get_performance_counter = SDL_GetPerformanceCounter;
	macos_platform.get_performance_frequency = SDL_GetPerformanceFrequency;

	macos_platform.file_delete = macos_file_delete;
	macos_platform.file_exists = macos_file_exists;

	macos_platform.dir_create = macos_dir_create;
	macos_platform.dir_delete = macos_dir_delete;
	macos_platform.dir_exists = macos_dir_exists;

	macos_platform.stream_from_file = macos_stream_from_file;
	macos_platform.stream_from_memory = macos_stream_from_memory;
	macos_platform.stream_from_const_memory = macos_stream_from_const_memory;
	macos_platform.stream_read = macos_stream_read;
	macos_platform.stream_write = macos_stream_write;
	macos_platform.stream_seek = macos_stream_seek;
	macos_platform.stream_size = macos_stream_size;
	macos_platform.stream_position = macos_stream_position;
	macos_platform.stream_close = macos_stream_close;

	macos_platform.open_in_explorer = macos_open_in_explorer;

	macos_platform.create_vulkan_surface = macos_create_vulkan_surface;
	macos_platform.destroy_vulkan_surface = macos_destroy_vulkan_surface;
	macos_platform.get_vulkan_instance_extensions = SDL_Vulkan_GetInstanceExtensions;
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

	macos_sdl_window = SDL_CreateWindow(
		DEFAULT_WINDOW_TITLE,
		DEFAULT_WINDOW_WIDTH,
		DEFAULT_WINDOW_HEIGHT,
		window_flags
	);

	if (!macos_sdl_window) {
		debug_log_crash("Failed to create SDL window: %s", SDL_GetError());
		return -1;
	}

	init_platform();

	Platform prev_st = macos_platform;

	App app(macos_platform);
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
				macos_platform.kb_down[ev.key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				macos_platform.kb_down[ev.key.scancode] = false;
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				macos_platform.mb_down[ev.button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				macos_platform.mb_down[ev.button.button] = false;
				break;

			case SDL_EVENT_MOUSE_MOTION: {
				float spx = 0.f;
				float spy = 0.f;

				SDL_GetGlobalMouseState(&spx, &spy);

				macos_platform.mouse_position = Vec2(ev.motion.x, ev.motion.y);
				macos_platform.mouse_delta = Vec2(ev.motion.xrel, ev.motion.yrel);
				macos_platform.mouse_screen_position = Vec2(spx, spy);

				break;
			}

			case SDL_EVENT_MOUSE_WHEEL:
				macos_platform.mouse_wheel = Vec2(ev.wheel.x, ev.wheel.y);
				break;
			}
		}

		for (int i = 0; i < KEYBOARD_KEY_MAX_ENUM; i++) {
			macos_platform.kb_pressed [i] =  macos_platform.kb_down[i] && !prev_st.kb_down[i];
			macos_platform.kb_released[i] = !macos_platform.kb_down[i] &&  prev_st.kb_down[i];
		}

		for (int i = 0; i < MBUTTON_MAX_ENUM; i++) {
			macos_platform.mb_pressed [i] =  macos_platform.mb_down[i] && !prev_st.mb_down[i];
			macos_platform.mb_released[i] = !macos_platform.mb_down[i] &&  prev_st.mb_down[i];
		}

		for (int i = 0; i < MAX_GAMEPADS; i++) {
			GamepadState *st = &macos_platform.gamepads[i];
			GamepadState *p_st = &prev_st.gamepads[i];

			for (int j = 0; j < GAMEPAD_BUTTON_MAX_ENUM; j++) {
				st->pressed [i] =  st->down[i] && !p_st->down[i];
				st->released[i] = !st->down[i] &&  p_st->down[i];
			}
		}

		if (app.tick())
			break;

		prev_st = macos_platform;
	}

	app.destroy();

	macos_close_all_gamepads();

	SDL_DestroyWindow(macos_sdl_window);
	SDL_Quit();

	return 0;
}
