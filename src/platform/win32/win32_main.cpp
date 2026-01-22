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

#include <mutex>

#include "platform/platform.h"
#include "math/vec2.h"
#include "core/types.h"
#include "job/job.h"

#include "app.h"

namespace
{

SDL_Window *win32_sdl_window = nullptr;
inp::InputState win32_input_st = {};
std::atomic<bool> app_running { true };
std::mutex input_mutex;

}

void platform::set_window_title(const char *title)
{
	SDL_SetWindowTitle(win32_sdl_window, title);
}

void platform::get_window_size(int *width, int *height)
{
	SDL_GetWindowSize(win32_sdl_window, width, height);
}

void get_window_size_in_pixels(int *pixel_width, int *pixel_height)
{
	SDL_GetWindowSizeInPixels(win32_sdl_window, pixel_width, pixel_height);
}

void platform::set_window_size(u32 width, u32 height)
{
	SDL_SetWindowSize(win32_sdl_window, width, height);
}

void platform::set_window_fullscreen(bool b)
{
	SDL_SetWindowFullscreen(win32_sdl_window, b);
}

void platform::set_window_borderless(bool b)
{
	SDL_SetWindowBordered(win32_sdl_window, !b);
}

void platform::set_mouse_position(u32 x, u32 y)
{
	win32_input_st.mouse_position = Vec2(x, y);

	SDL_WarpMouseInWindow(win32_sdl_window, x, y);

	SDL_GetGlobalMouseState(
		&win32_input_st.mouse_screen_position.x,
		&win32_input_st.mouse_screen_position.y
	);
}

void platform::set_mouse_visible(bool visible)
{
	if (visible)
		SDL_ShowCursor();
	else
		SDL_HideCursor();
}

void platform::set_mouse_locked(bool locked)
{
	SDL_SetWindowRelativeMouseMode(win32_sdl_window, locked);
}

void platform::set_window_opacity(float opacity)
{
	SDL_SetWindowOpacity(win32_sdl_window, opacity);
}

u64 platform::get_ticks()
{
	return SDL_GetTicks();
}

u64 platform::get_performance_counter()
{
	return SDL_GetPerformanceCounter();
}

u64 platform::get_performance_frequency()
{
	return SDL_GetPerformanceFrequency();
}

void *platform::create_thread(ulong (*entry)(void *param), void *param)
{
	return CreateThread(NULL, 0, entry, param, 0, NULL);
}

void platform::join_thread(void *handle)
{
	WaitForSingleObject(handle, INFINITE);
	CloseHandle(handle);
}

void platform::detach_thread(void *handle)
{
	CloseHandle(handle);
}

void *platform::convert_thread_to_fiber()
{
	return ConvertThreadToFiber(nullptr);
}

int platform::convert_fiber_to_thread()
{
	return ConvertFiberToThread();
}

void *platform::create_fiber(u32 stack_size, fiber_entry_point_fn entry, void *param)
{
	return CreateFiber(stack_size, entry, param);
}

void platform::delete_fiber(void *handle)
{
	DeleteFiber(handle);
}

void platform::switch_to_fiber(void *handle)
{
	SwitchToFiber(handle);
}

bool platform::file_delete(const char *path)
{
	return std::filesystem::remove(path);
}

bool platform::file_exists(const char *path)
{
	return std::filesystem::exists(path);
}

bool platform::dir_create(const char *path)
{
	return std::filesystem::create_directory(path);
}

bool platform::dir_delete(const char *path)
{
	return std::filesystem::remove_all(path) > 0;
}

bool platform::dir_exists(const char *path)
{
	return std::filesystem::is_directory(path);
}

void *platform::stream_from_file(const char *path, const char *mode)
{
	return SDL_IOFromFile(path, mode);
}

void *platform::stream_from_memory(void *data, u64 size)
{
	return SDL_IOFromMem(data, size);
}

void *platform::stream_from_const_memory(const void *data, u64 size)
{
	return SDL_IOFromConstMem(data, size);
}

s64 platform::stream_read(void *stream, void *dst, u64 size)
{
	return SDL_ReadIO((SDL_IOStream *)stream, dst, size);
}

s64 platform::stream_write(void *stream, const void *src, u64 size)
{
	return SDL_WriteIO((SDL_IOStream *)stream, src, size);
}

s64 platform::stream_seek(void *stream, s64 offset)
{
	return SDL_SeekIO((SDL_IOStream *)stream, offset, SDL_IO_SEEK_SET);
}

s64 platform::stream_size(void *stream)
{
	return SDL_GetIOSize((SDL_IOStream *)stream);
}

s64 platform::stream_position(void *stream)
{
	return SDL_TellIO((SDL_IOStream *)stream);
}

bool platform::stream_close(void *stream)
{
	return SDL_CloseIO((SDL_IOStream *)stream);
}

void platform::open_in_explorer(const char *path)
{
	ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOWDEFAULT);
}

bool platform::create_vulkan_surface(void *instance, void *surface_pointer)
{
	return SDL_Vulkan_CreateSurface(win32_sdl_window, (VkInstance)instance, nullptr, (VkSurfaceKHR *)surface_pointer);
}

void platform::destroy_vulkan_surface(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface((VkInstance)instance, (VkSurfaceKHR)surface, nullptr);
}

const char *const *platform::get_vulkan_instance_extensions(u32 *count)
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

static void reconnect_all_gamepads()
{
	// TODO
}

static void win32_close_all_gamepads()
{
	// TODO
}

static JOB_ENTRY_POINT(root_entry_point)
{
	App *app = (App *)param;

	app->init();

	debug_log("Entering main loop...");

	inp::InputState prev_input_st = {};

	while (app_running.load()) {
		inp::InputState curr_input_st = {};
		{
			std::lock_guard<std::mutex> lock(input_mutex);
			curr_input_st = win32_input_st;

			// Reset accumulated values.
			win32_input_st.mouse_delta = Vec2::zero();
			win32_input_st.mouse_wheel = Vec2::zero();
		}

		for (int i = 0; i < inp::KEYBOARD_KEY_MAX_ENUM; i++) {
			curr_input_st.kb_pressed [i] =  curr_input_st.kb_down[i] && !prev_input_st.kb_down[i];
			curr_input_st.kb_released[i] = !curr_input_st.kb_down[i] &&  prev_input_st.kb_down[i];
		}

		for (int i = 0; i < inp::MBUTTON_MAX_ENUM; i++) {
			curr_input_st.mb_pressed [i] =  curr_input_st.mb_down[i] && !prev_input_st.mb_down[i];
			curr_input_st.mb_released[i] = !curr_input_st.mb_down[i] &&  prev_input_st.mb_down[i];
		}

		for (int i = 0; i < MAX_GAMEPADS; i++) {
			inp::GamepadState *st   = &curr_input_st.gamepads[i];
			inp::GamepadState *p_st = &prev_input_st.gamepads[i];

			for (int j = 0; j < inp::GAMEPAD_BUTTON_MAX_ENUM; j++) {
				st->pressed [j] =  st->down[j] && !p_st->down[j];
				st->released[j] = !st->down[j] &&  p_st->down[j];
			}
		}

		if (app->tick(curr_input_st))
			app_running = false;

		prev_input_st = curr_input_st;
	}

	app->destroy();
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

	// Init platform.
	platform::set_window_title(DEFAULT_WINDOW_TITLE);
	platform::set_window_size(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
	platform::set_window_opacity(1.f);
	platform::set_window_fullscreen(false);
	platform::set_window_borderless(false);
	platform::set_mouse_visible(true);
	platform::set_mouse_locked(false);

	/*
	 * As a note to myself and to anyone reading this.
	 *
	 * Because I am using a fiber-based job system architecture, we need
	 * the "main" thread to be able to yield, to wait for jobs to complete.
	 * 
	 * This means that the main app code has to be actually ran from a "root"
	 * job that we launch here.
	 */
	
	App app;

	job::init(std::thread::hardware_concurrency() - 1);
	job::kick_job(job::JobDecl(root_entry_point, &app), nullptr);

	while (app_running.load()) {
		SDL_Event ev = {};

		while (SDL_PollEvent(&ev)) {
			std::lock_guard<std::mutex> lock(input_mutex);

			switch (ev.type) {
				case SDL_EVENT_QUIT:
					app_running.store(false);
					break;

				case SDL_EVENT_KEY_DOWN:
					win32_input_st.kb_down[ev.key.scancode] = true;
					break;

				case SDL_EVENT_KEY_UP:
					win32_input_st.kb_down[ev.key.scancode] = false;
					break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					win32_input_st.mb_down[ev.button.button] = true;
					break;

				case SDL_EVENT_MOUSE_BUTTON_UP:
					win32_input_st.mb_down[ev.button.button] = false;
					break;

				case SDL_EVENT_MOUSE_MOTION: {
					float spx = 0.f;
					float spy = 0.f;

					SDL_GetGlobalMouseState(&spx, &spy);

					win32_input_st.mouse_position = Vec2(ev.motion.x, ev.motion.y);
					win32_input_st.mouse_screen_position = Vec2(spx, spy);
					win32_input_st.mouse_delta += Vec2(ev.motion.xrel, ev.motion.yrel);
				} break;

				case SDL_EVENT_MOUSE_WHEEL:
					win32_input_st.mouse_wheel += Vec2(ev.wheel.x, ev.wheel.y);
					break;
			}
		}

		JOB_SPIN_PAUSE();
	}

	job::shutdown();

	win32_close_all_gamepads();

	SDL_DestroyWindow(win32_sdl_window);
	SDL_Quit();

	return 0;
}
