#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "assert.h"
#include "abstraction_layer.h"
#include "program_options.h"
#include "platform.h"

internal void AppNullStub(Platform *platform) { }

global SDL_Window *window = 0;
global Platform global_platform = {0};
global InputState prev_input_st = {0};

typedef struct MacOSAppCode
{
	void *handle;
	b32 is_valid;
	
	void (*AppInit)(Platform *);
	void (*AppUpdate)(Platform *);
	void (*AppDestroy)(Platform *);
	void (*AppBeforeHotReload)(Platform *);
	void (*AppAfterHotReload)(Platform *);
}
MacOSAppCode;

#include <copyfile.h>

internal void
LoadAppCode(MacOSAppCode *app_code)
{
	app_code->is_valid = 0;
	
	app_code->handle = dlopen("build/app.dylib", RTLD_NOW | RTLD_LOCAL);
	
	if(app_code->handle)
	{
		app_code->AppInit            = dlsym(app_code->handle, "AppInit");
		app_code->AppUpdate          = dlsym(app_code->handle, "AppUpdate");
		app_code->AppDestroy         = dlsym(app_code->handle, "AppDestroy");
		app_code->AppBeforeHotReload = dlsym(app_code->handle, "AppBeforeHotReload");
		app_code->AppAfterHotReload  = dlsym(app_code->handle, "AppAfterHotReload");
		
		app_code->is_valid = (app_code->AppInit &&
							  app_code->AppUpdate &&
							  app_code->AppDestroy &&
							  app_code->AppAfterHotReload &&
							  app_code->AppBeforeHotReload);
	}
}

internal void
UnloadAppCode(MacOSAppCode *app_code)
{
	app_code->AppInit = AppNullStub;
	app_code->AppUpdate = AppNullStub;
	app_code->AppDestroy = AppNullStub;
	app_code->AppBeforeHotReload = AppNullStub;
	app_code->AppAfterHotReload = AppNullStub;
	
	app_code->is_valid = 0;
	
	if(app_code->handle)
	{
		dlclose(app_code->handle);
		app_code->handle = 0;
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
		
		global_platform.window_opacity = 1.f;
		
		global_platform.fullscreen = 0;
		global_platform.borderless = 0;
		
		global_platform.target_fps = 120;
		global_platform.current_time = 0.f;
		
		global_platform.cursor_visible = 1;
		global_platform.cursor_locked = 0;
		
		global_platform.exit = 0;
		
		global_platform.GetVulkanInstanceExtensions = SDL_Vulkan_GetInstanceExtensions;
		global_platform.CreateVulkanSurface = CreateVulkanSurface;
		
		global_platform.GetTicks = SDL_GetTicks;
		global_platform.GetPerformanceCounter = SDL_GetPerformanceCounter;
		global_platform.GetPerformanceFrequency = SDL_GetPerformanceFrequency;
	}
	
	MacOSAppCode app_code = {0};
	LoadAppCode(&app_code);
	
	time_t last_reload = 0;
	
	app_code.AppInit(&global_platform);
	
	while(!global_platform.exit)
	{
		SDL_Event ev = {0};
		
		while(SDL_PollEvent(&ev))
		{
			switch(ev.type)
			{
				case SDL_EVENT_QUIT:
				{
					global_platform.exit = 1;
				}
				break;
				
				case SDL_EVENT_WINDOW_RESIZED:
				{
				}
				break;
				
				case SDL_EVENT_KEY_DOWN:
				{
					global_platform.input.kb_down[ev.key.scancode] = 1;
					global_platform.input.kb_pressed[ev.key.scancode] = !prev_input_st.kb_down[ev.key.scancode];
				}
				break;
				
				case SDL_EVENT_KEY_UP:
				{
					global_platform.input.kb_down[ev.key.scancode] = 0;
					global_platform.input.kb_released[ev.key.scancode] = prev_input_st.kb_down[ev.key.scancode];
				}
				break;
				
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					global_platform.input.mb_down[ev.button.button] = 1;
					global_platform.input.mb_pressed[ev.button.button] = !prev_input_st.mb_down[ev.button.button];
				}
				break;
				
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					global_platform.input.mb_down[ev.button.button] = 0;
					global_platform.input.mb_released[ev.button.button] = prev_input_st.mb_down[ev.button.button];
				}
				break;
				
				case SDL_EVENT_MOUSE_MOTION:
				{
					f32 spx = 0.f, spy = 0.f;
					
					SDL_GetGlobalMouseState(&spx, &spy);
					
					global_platform.input.mouse_position = v2(ev.motion.x, ev.motion.y);
					global_platform.input.mouse_delta = v2(ev.motion.xrel, ev.motion.yrel);
					global_platform.input.mouse_screen_position = v2(spx, spy);
				}
				break;
				
				case SDL_EVENT_MOUSE_WHEEL:
				{
					global_platform.input.mouse_wheel = v2(ev.wheel.x, ev.wheel.y);
				}
				break;
			}
		}
		
		Platform prev_st = global_platform;
		
		app_code.AppUpdate(&global_platform);
		
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
			if(global_platform.window_width >= 0 && global_platform.window_height >= 0)
			{
				SDL_SetWindowSize(window, global_platform.window_width, global_platform.window_height);
			}
		}
		
		prev_input_st = global_platform.input;
		
		struct stat st_reload = {0};
		stat("build/app.dylib", &st_reload);
		
		if(st_reload.st_mtime != last_reload)
		{
			app_code.AppBeforeHotReload(&global_platform);
			
			UnloadAppCode(&app_code);
			LoadAppCode(&app_code);
			
			app_code.AppAfterHotReload(&global_platform);
			
			last_reload = st_reload.st_mtime;
		}
	}
	
	app_code.AppDestroy(&global_platform);
	
	UnloadAppCode(&app_code);
	
	CloseAllGamepads();
	
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	free(global_platform.permanent_memory);
	free(global_platform.transient_memory);
	free(global_platform.scratch_memory[0]);
	free(global_platform.scratch_memory[1]);
	
	return 0;
}
