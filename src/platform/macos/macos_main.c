#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <copyfile.h>

#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "platform/platform.h"

#include "core/core_math.h"
#include "core/core_types.h"

void app_null_stub(struct platform *platform_) { }
bool app_tick_stub(struct platform *platform_) { return false; }

static SDL_Window *window = NULL;
static struct platform platform = {0};

struct macos_code {
	void *handle;
	time_t last_modified_time;

	void (*init)(struct platform *);
	bool (*tick)(struct platform *);
	void (*destroy)(struct platform *);
	void (*hot_load)(struct platform *);
	void (*hot_unload)(struct platform *);
};

static void macos_load_code(struct macos_code *app_code, const char *source_dylib)
{
	const char *dylib_name_hot = "build/hot_reload.dylib";

	if (copyfile(source_dylib, dylib_name_hot, NULL, COPYFILE_DATA) != 0)
		perror("Failed to copy game dylib for hot reload");

	app_code->handle = dlopen(dylib_name_hot, RTLD_LAZY);

	struct stat file_stat = {0};
	stat(source_dylib, &file_stat);
	
	app_code->last_modified_time = file_stat.st_mtime;

	if (app_code->handle) {
		app_code->init       = (void (*)(struct platform *))dlsym(app_code->handle, "app_init");
		app_code->tick       = (bool (*)(struct platform *))dlsym(app_code->handle, "app_tick");
		app_code->destroy    = (void (*)(struct platform *))dlsym(app_code->handle, "app_destroy");
		app_code->hot_load   = (void (*)(struct platform *))dlsym(app_code->handle, "app_hot_load");
		app_code->hot_unload = (void (*)(struct platform *))dlsym(app_code->handle, "app_hot_unload");
	}
}

static void macos_unload_code(struct macos_code *app_code)
{
	app_code->init       = app_null_stub;
	app_code->tick       = app_tick_stub;
	app_code->destroy    = app_null_stub;
	app_code->hot_load   = app_null_stub;
	app_code->hot_unload = app_null_stub;

	if (app_code->handle) {
		dlclose(app_code->handle);
		app_code->handle = NULL;
	}
}

static bool macos_create_vulkan_surface(void *instance, void *surface_pointer)
{
	return SDL_Vulkan_CreateSurface(window, (VkInstance)instance, NULL, (VkSurfaceKHR *)surface_pointer);
}

static void macos_destroy_vulkan_surface(void *instance, void *surface)
{
	SDL_Vulkan_DestroySurface((VkInstance)instance, (VkSurfaceKHR)surface, NULL);
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
	SDL_SetWindowSize(window, width, height);

	SDL_GetWindowSizeInPixels(window,
				  &platform.window_pixel_width,
				  &platform.window_pixel_height);
}

static void macos_set_window_fullscreen(bool b)
{
	SDL_SetWindowFullscreen(window, b);
}

static void macos_set_window_borderless(bool b)
{
	SDL_SetWindowBordered(window, !b);
}

static void macos_set_mouse_position(u32 x, u32 y)
{
	platform.mouse_position = v2(x, y);

	SDL_WarpMouseInWindow(window, x, y);
	
	SDL_GetGlobalMouseState(&platform.mouse_screen_position.x,
				&platform.mouse_screen_position.y);
}

static void init_platform()
{
	platform.memory = malloc(MEMORY_SIZE);
	platform.memory_size = MEMORY_SIZE;

	platform.window_width = DEFAULT_WINDOW_WIDTH;
	platform.window_height = DEFAULT_WINDOW_HEIGHT;

	SDL_GetWindowSizeInPixels(window,
				  &platform.window_pixel_width,
				  &platform.window_pixel_height);

	platform.window_opacity = 1.f;

	platform.fullscreen = false;
	platform.borderless = false;

	platform.target_fps = 120;
	platform.current_time = 0.f;

	platform.cursor_visible = true;
	platform.cursor_locked = false;

	platform.set_window_size = macos_set_window_size;
	platform.set_window_fullscreen = macos_set_window_fullscreen;
	platform.set_window_borderless = macos_set_window_borderless;

	platform.set_mouse_position = macos_set_mouse_position;

	platform.get_ticks = SDL_GetTicks;
	platform.get_performance_counter = SDL_GetPerformanceCounter;
	platform.get_performance_frequency = SDL_GetPerformanceFrequency;

	platform.create_vulkan_surface = macos_create_vulkan_surface;
	platform.destroy_vulkan_surface = macos_destroy_vulkan_surface;
	platform.get_vulkan_instance_extensions = SDL_Vulkan_GetInstanceExtensions;
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

	window = SDL_CreateWindow(WINDOW_TITLE,
				  DEFAULT_WINDOW_WIDTH,
				  DEFAULT_WINDOW_HEIGHT,
				  window_flags);

	if (!window) {
		debug_log_crash("Failed to create SDL window: %s", SDL_GetError());
		return -1;
	}
	
	init_platform();
	struct platform prev_st = platform;

	// Load in our dynamically linked code seperately.
	const char *source_dylib = "build/libgame.dylib";
	struct macos_code app_code = {0};
	macos_load_code(&app_code, source_dylib);

	// Init app.
	app_code.init(&platform);
        
	debug_log("Entering main loop...");

	bool running = true;
	
	while (running) {
		struct stat file_stat = {0};
		stat(source_dylib, &file_stat);
		
		if (app_code.last_modified_time != file_stat.st_mtime) {
			app_code.hot_unload(&platform);
			macos_unload_code(&app_code);
			macos_load_code(&app_code, source_dylib);
			app_code.hot_load(&platform);
			debug_log("Hot reloaded!");
		}
		
		SDL_Event ev = {0};

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_EVENT_QUIT:
				running = false;
				break;
				
			case SDL_EVENT_WINDOW_RESIZED:
				break;

			case SDL_EVENT_KEY_DOWN:
				platform.kb_down[ev.key.scancode] = true;
				break;

			case SDL_EVENT_KEY_UP:
				platform.kb_down[ev.key.scancode] = false;
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				platform.mb_down[ev.button.button] = true;
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				platform.mb_down[ev.button.button] = false;
				break;

			case SDL_EVENT_MOUSE_MOTION: {
				float spx = 0.f;
				float spy = 0.f;

				SDL_GetGlobalMouseState(&spx, &spy);

				platform.mouse_position = v2(ev.motion.x, ev.motion.y);
				platform.mouse_delta = v2(ev.motion.xrel, ev.motion.yrel);
				platform.mouse_screen_position = v2(spx, spy);

				break;
			}

			case SDL_EVENT_MOUSE_WHEEL:
				platform.mouse_wheel = v2(ev.wheel.x, ev.wheel.y);
				break;
			}
		}

		for (u32 i = 0; i < KEYBOARD_KEY_max_enum; i++) {
			platform.kb_pressed [i] =  platform.kb_down[i] && !prev_st.kb_down[i];
			platform.kb_released[i] = !platform.kb_down[i] &&  prev_st.kb_down[i];
		}

		for (u32 i = 0; i < MBUTTON_max_enum; i++) {
			platform.mb_pressed [i] =  platform.mb_down[i] && !prev_st.mb_down[i];
			platform.mb_released[i] = !platform.mb_down[i] &&  prev_st.mb_down[i];
		}

		for (u32 i = 0; i < MAX_GAMEPADS; i++) {
			struct gamepad_st *st = platform.gamepads + i;
			struct gamepad_st *p_st = prev_st.gamepads + i;
			for (u32 j = 0; j < GAMEPAD_BUTTON_max_enum; j++) {
				st->pressed [i] =  st->down[i] && !p_st->down[i];
				st->released[i] = !st->down[i] &&  p_st->down[i];
			}
		}

		if (app_code.tick(&platform))
			break;

		prev_st = platform;
	}

	app_code.destroy(&platform);

	macos_unload_code(&app_code);
	macos_close_all_gamepads();

	SDL_DestroyWindow(window);
	SDL_Quit();

	free(platform.memory);

	return 0;
}
