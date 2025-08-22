#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "assert.h"
#include "abstraction_layer.h"
#include "program_constants.h"
#include "platform.h"

internal void CoreNullStub(Platform *platform) { }

global SDL_Window *window = 0;
global Platform global_platform = {0};

typedef struct MacOSCoreCode
{
	void *handle;
	
	void (*CoreInit)(Platform *);
	void (*CoreUpdate)(Platform *);
	void (*CoreDestroy)(Platform *);
	void (*CoreBeforeHotReload)(Platform *);
	void (*CoreAfterHotReload)(Platform *);
}
MacOSCoreCode;

internal void
LoadCoreCode(MacOSCoreCode *core_code)
{
	core_code->handle = dlopen("build/core.dylib", RTLD_NOW | RTLD_LOCAL);
	
	if(core_code->handle)
	{
		core_code->CoreInit            = dlsym(core_code->handle, "CoreInit");
		core_code->CoreUpdate          = dlsym(core_code->handle, "CoreUpdate");
		core_code->CoreDestroy         = dlsym(core_code->handle, "CoreDestroy");
		core_code->CoreBeforeHotReload = dlsym(core_code->handle, "CoreBeforeHotReload");
		core_code->CoreAfterHotReload  = dlsym(core_code->handle, "CoreAfterHotReload");
	}
}

internal void
UnloadCoreCode(MacOSCoreCode *core_code)
{
	core_code->CoreInit            = CoreNullStub;
	core_code->CoreUpdate          = CoreNullStub;
	core_code->CoreDestroy         = CoreNullStub;
	core_code->CoreBeforeHotReload = CoreNullStub;
	core_code->CoreAfterHotReload  = CoreNullStub;
	
	if(core_code->handle)
	{
		dlclose(core_code->handle);
		core_code->handle = 0;
	}
}

internal b32
CreateVulkanSurface(void *instance, void *surface)
{
	return SDL_Vulkan_CreateSurface(window, (VkInstance)instance, 0, (VkSurfaceKHR *)surface);
}

internal void
ReconnectAllGamepads()
{
	// TODO(kp)
}

internal void
CloseAllGamepads()
{
	// TODO(kp)
}

i32
main(void)
{
	u32 init_flags =
		SDL_INIT_VIDEO |
		SDL_INIT_AUDIO |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMEPAD |
		SDL_INIT_HAPTIC |
		SDL_INIT_EVENTS |
		SDL_INIT_SENSOR |
		SDL_INIT_CAMERA;
	
	if(!SDL_Init(init_flags))
	{
		DebugLogCrash("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}
	
	u64 window_flags =
		SDL_WINDOW_VULKAN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY;
	
	window = SDL_CreateWindow(WINDOW_TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, window_flags);
	
	if(!window)
	{
		DebugLogCrash("Failed to create SDL window: %s", SDL_GetError());
		return -1;
	}
	
	// NOTE(kp): Init platform.
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
		
		global_platform.GetVulkanInstanceExtensions = SDL_Vulkan_GetInstanceExtensions;
		global_platform.CreateVulkanSurface = CreateVulkanSurface;
		
		global_platform.GetTicks = SDL_GetTicks;
		global_platform.GetPerformanceCounter = SDL_GetPerformanceCounter;
		global_platform.GetPerformanceFrequency = SDL_GetPerformanceFrequency;
	}
	
	Platform prev_st = global_platform;
	
	// NOTE(kp): Load in our dynamically linked code seperately.
	MacOSCoreCode core_code = {0};
	LoadCoreCode(&core_code);
	core_code.CoreInit(&global_platform);
	
	struct stat st_reload = {0};
	stat("build/core.dylib", &st_reload);
	time_t last_reload = st_reload.st_mtime;
	
	DebugLog("Entering main loop...");
	
	while(!global_platform.exit)
	{
		// NOTE(kp): Only reload our dynamic library when we detect the file has been changed.
		//           I.e: recompiled.
		stat("build/core.dylib", &st_reload);
		
		if(st_reload.st_mtime != last_reload)
		{
			core_code.CoreBeforeHotReload(&global_platform);
			
			UnloadCoreCode(&core_code);
			LoadCoreCode(&core_code);
			
			core_code.CoreAfterHotReload(&global_platform);
			
			last_reload = st_reload.st_mtime;
			
			DebugLog("Hot reloaded!");
		}
		
		SDL_Event ev = {0};
		
		while(SDL_PollEvent(&ev))
		{
			switch(ev.type)
			{
				case SDL_EVENT_QUIT:
				{
					global_platform.exit = true;
				}
				break;
				
				case SDL_EVENT_WINDOW_RESIZED:
				{
				}
				break;
				
				case SDL_EVENT_KEY_DOWN:
				{
					global_platform.kb_down[ev.key.scancode] = true;
					global_platform.kb_pressed[ev.key.scancode] = !prev_st.kb_down[ev.key.scancode];
				}
				break;
				
				case SDL_EVENT_KEY_UP:
				{
					global_platform.kb_down[ev.key.scancode] = false;
					global_platform.kb_released[ev.key.scancode] = prev_st.kb_down[ev.key.scancode];
				}
				break;
				
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					global_platform.mb_down[ev.button.button] = true;
					global_platform.mb_pressed[ev.button.button] = !prev_st.mb_down[ev.button.button];
				}
				break;
				
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					global_platform.mb_down[ev.button.button] = false;
					global_platform.mb_released[ev.button.button] = prev_st.mb_down[ev.button.button];
				}
				break;
				
				case SDL_EVENT_MOUSE_MOTION:
				{
					f32 spx = 0.f, spy = 0.f;
					
					SDL_GetGlobalMouseState(&spx, &spy);
					
					global_platform.mouse_position = v2(ev.motion.x, ev.motion.y);
					global_platform.mouse_delta = v2(ev.motion.xrel, ev.motion.yrel);
					global_platform.mouse_screen_position = v2(spx, spy);
				}
				break;
				
				case SDL_EVENT_MOUSE_WHEEL:
				{
					global_platform.mouse_wheel = v2(ev.wheel.x, ev.wheel.y);
				}
				break;
			}
		}
		
		core_code.CoreUpdate(&global_platform);
		
		if(global_platform.fullscreen != prev_st.fullscreen)
		{
			SDL_SetWindowFullscreen(window, global_platform.fullscreen);
		}
		
		if(global_platform.borderless != prev_st.borderless)
		{
			SDL_SetWindowBordered(window, global_platform.borderless);
		}
		
		if(global_platform.window_width != prev_st.window_width ||
		   global_platform.window_height != prev_st.window_height)
		{
			if(global_platform.window_width >= 0 &&
			   global_platform.window_height >= 0)
			{
				SDL_SetWindowSize(window,
								  global_platform.window_width,
								  global_platform.window_height);
				
				SDL_GetWindowSizeInPixels(window,
										  &global_platform.window_pixel_width,
										  &global_platform.window_pixel_height);
			}
		}
		
		prev_st = global_platform;
	}
	
	core_code.CoreDestroy(&global_platform);
	
	UnloadCoreCode(&core_code);
	
	CloseAllGamepads();
	
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	free(global_platform.permanent_memory);
	free(global_platform.transient_memory);
	free(global_platform.scratch_memory[0]);
	free(global_platform.scratch_memory[1]);
	
	return 0;
}
