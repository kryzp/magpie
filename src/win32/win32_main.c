#include <windows.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <stdio.h>

#include "kp.h"
#include "platform.h"

typedef void (*CoreFunctionType)(Platform *);

internal void CoreNullStub(Platform *platform)
{
}

global SDL_Window *window = 0;
global Platform global_platform = {0};

typedef struct Win32CoreCode {
	HMODULE handle;
	FILETIME last_write_time;

	void (*CoreInit)(Platform *platform);
	void (*CoreTick)(Platform *platform);
	void (*CoreDestroy)(Platform *platform);
	void (*CoreBeforeHotReload)(Platform *platform);
	void (*CoreAfterHotReload)(Platform *platform);
} Win32CoreCode;

internal FILETIME Win32GetLastWriteTime(const char *filename)
{
	FILETIME last_write_time = {0};

	WIN32_FIND_DATA find_data = {0};
	HANDLE find_handle = FindFirstFileA(filename, &find_data);

	if (find_handle != INVALID_HANDLE_VALUE) {
		FindClose(find_handle);
		last_write_time = find_data.ftLastWriteTime;
	}

	return last_write_time;
}

internal void Win32LoadCoreCode(Win32CoreCode *core_code, const char *source_dll)
{
	const char *dll_name_hot = "build/hot_reload.dll";

	core_code->last_write_time = Win32GetLastWriteTime(source_dll);

	CopyFile(source_dll, dll_name_hot, FALSE);
	core_code->handle = LoadLibraryA(dll_name_hot);

	if (core_code->handle) {
		core_code->CoreInit            = (CoreFunctionType)GetProcAddress(core_code->handle, "CoreInit");
		core_code->CoreTick            = (CoreFunctionType)GetProcAddress(core_code->handle, "CoreTick");
		core_code->CoreDestroy         = (CoreFunctionType)GetProcAddress(core_code->handle, "CoreDestroy");
		core_code->CoreBeforeHotReload = (CoreFunctionType)GetProcAddress(core_code->handle, "CoreBeforeHotReload");
		core_code->CoreAfterHotReload  = (CoreFunctionType)GetProcAddress(core_code->handle, "CoreAfterHotReload");
	}
}

internal void Win32UnloadCoreCode(Win32CoreCode *core_code)
{
	core_code->CoreInit = CoreNullStub;
	core_code->CoreTick = CoreNullStub;
	core_code->CoreDestroy = CoreNullStub;
	core_code->CoreBeforeHotReload = CoreNullStub;
	core_code->CoreAfterHotReload = CoreNullStub;

	if (core_code->handle) {
		FreeLibrary(core_code->handle);
		core_code->handle = 0;
	}
}

internal b32 Win32CreateVulkanSurface(void *instance, void *surface)
{
	return SDL_Vulkan_CreateSurface(window, (VkInstance)instance, 0, (VkSurfaceKHR *)surface);
}

internal void Win32ReconnectAllGamepads()
{
	// TODO(kp)
}

internal void Win32CloseAllGamepads()
{
	// TODO(kp)
}

internal void Win32SetWindowSize(u32 w, u32 h)
{
	SDL_SetWindowSize(window, w, h);

	SDL_GetWindowSizeInPixels(window,
				  &global_platform.window_pixel_width,
				  &global_platform.window_pixel_height);
}

internal void Win32SetWindowFullscreen(b32 b)
{
	SDL_SetWindowFullscreen(window, b);
}

internal void Win32SetWindowBorderless(b32 b)
{
	SDL_SetWindowBordered(window, !b);
}

internal void Win32SetMousePosition(u32 x, u32 y)
{
	SDL_WarpMouseInWindow(window, x, y);
	
	global_platform.mouse_position = v2(x, y);

	SDL_GetGlobalMouseState(&global_platform.mouse_screen_position.x,
				&global_platform.mouse_screen_position.y);
}

i32 main(void)
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
		DebugLogCrash("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN |
				       SDL_WINDOW_HIGH_PIXEL_DENSITY;

	window = SDL_CreateWindow(WINDOW_TITLE, DEFAULT_WINDOW_WIDTH,
				  DEFAULT_WINDOW_HEIGHT, window_flags);

	if (!window) {
		DebugLogCrash("Failed to create SDL window: %s", SDL_GetError());
		return -1;
	}

	//  Init platform.
	{
		global_platform.permanent_memory_size = PERMANENT_MEMORY_SIZE;
		global_platform.transient_memory_size = TRANSIENT_MEMORY_SIZE;
		global_platform.scratch_memory_size = SCRATCH_MEMORY_SIZE;

		global_platform.permanent_memory = malloc(global_platform.permanent_memory_size);
		global_platform.transient_memory = malloc(global_platform.transient_memory_size);
		global_platform.scratch_memory[0] = malloc(global_platform.scratch_memory_size);
		global_platform.scratch_memory[1] = malloc(global_platform.scratch_memory_size);

		global_platform.window_width = DEFAULT_WINDOW_WIDTH;
		global_platform.window_height = DEFAULT_WINDOW_HEIGHT;

		SDL_GetWindowSizeInPixels(window,
					  &global_platform.window_pixel_width,
					  &global_platform.window_pixel_height);

		global_platform.window_opacity = 1.f;

		global_platform.fullscreen = false;
		global_platform.borderless = false;

		global_platform.target_fps = 120;
		global_platform.current_time = 0.f;

		global_platform.cursor_visible = true;
		global_platform.cursor_locked = false;

		global_platform.exit = false;

		global_platform.SetWindowSize = Win32SetWindowSize;
		global_platform.SetWindowFullscreen = Win32SetWindowFullscreen;
		global_platform.SetWindowBorderless = Win32SetWindowBorderless;

		global_platform.SetMousePosition = Win32SetMousePosition;

		global_platform.GetTicks = SDL_GetTicks;
		global_platform.GetPerformanceCounter = SDL_GetPerformanceCounter;
		global_platform.GetPerformanceFrequency = SDL_GetPerformanceFrequency;

		global_platform.GetVulkanInstanceExtensions = SDL_Vulkan_GetInstanceExtensions;
		global_platform.CreateVulkanSurface = Win32CreateVulkanSurface;
	}

	Platform prev_st = global_platform;

	//  Load in our dynamically linked code seperately.
	const char *source_dll = "build/core.dll";
	Win32CoreCode core_code = {0};
	Win32LoadCoreCode(&core_code, source_dll);
        
	DebugLog("Entering main loop...");

	core_code.CoreInit(&global_platform);

	while (!global_platform.exit) {
		FILETIME curr_write_time = Win32GetLastWriteTime(source_dll);
		
		if (CompareFileTime(&curr_write_time, &core_code.last_write_time) != 0) {
			core_code.CoreBeforeHotReload(&global_platform);

			Win32UnloadCoreCode(&core_code);
			Win32LoadCoreCode(&core_code, source_dll);

			core_code.CoreAfterHotReload(&global_platform);

			DebugLog("Hot reloaded!");
		}

		SDL_Event ev = {0};

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_EVENT_QUIT:
				global_platform.exit = true;
				break;
				
			case SDL_EVENT_WINDOW_RESIZED:
				break;

			case SDL_EVENT_KEY_DOWN:
				global_platform.kb_down[ev.key.scancode] = true;
				global_platform.kb_pressed[ev.key.scancode] = !prev_st.kb_down[ev.key.scancode];
				break;

			case SDL_EVENT_KEY_UP:
				global_platform.kb_down[ev.key.scancode] = false;
				global_platform.kb_released[ev.key.scancode] = prev_st.kb_down[ev.key.scancode];
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				global_platform.mb_down[ev.button.button] = true;
				global_platform.mb_pressed[ev.button.button] = !prev_st.mb_down[ev.button.button];
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				global_platform.mb_down[ev.button.button] = false;
				global_platform.mb_released[ev.button.button] = prev_st.mb_down[ev.button.button];
				break;

			case SDL_EVENT_MOUSE_MOTION: {
				f32 spx = 0.f;
				f32 spy = 0.f;

				SDL_GetGlobalMouseState(&spx, &spy);

				global_platform.mouse_position = v2(ev.motion.x, ev.motion.y);
				global_platform.mouse_delta = v2(ev.motion.xrel, ev.motion.yrel);
				global_platform.mouse_screen_position = v2(spx, spy);

				break;
			}

			case SDL_EVENT_MOUSE_WHEEL:
				global_platform.mouse_wheel = v2(ev.wheel.x, ev.wheel.y);
				break;
			}
		}

		core_code.CoreTick(&global_platform);

		prev_st = global_platform;
	}

	core_code.CoreDestroy(&global_platform);

	Win32UnloadCoreCode(&core_code);

	Win32CloseAllGamepads();

	SDL_DestroyWindow(window);
	SDL_Quit();

	free(global_platform.permanent_memory);
	free(global_platform.transient_memory);
	free(global_platform.scratch_memory[0]);
	free(global_platform.scratch_memory[1]);

	return 0;
}
