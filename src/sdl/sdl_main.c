#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <volk/volk.h>
#include "ext/vk_mem_alloc.h"

#define PLATFORM_MACOS

#include "program_options.h"
#include "abstraction_layer.h"
#include "platform.h"
#include "app.c"

global SDL_Window *window = 0;
global Platform platform = {0};
global InputState prev_input_st = {0};

internal const char * const*
GetVulkanInstanceExtensions_SDL3(u32 *count)
{
	return SDL_Vulkan_GetInstanceExtensions(count);
}

internal b32
CreateVulkanSurface_SDL3(void *instance, void *surface)
{
	return SDL_Vulkan_CreateSurface(window, (VkInstance)instance, 0, (VkSurfaceKHR *)surface);
}

internal void
ReconnectAllGamepads()
{
	// TODO
}

internal void
CloseAllGamepads()
{
	// TODO
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
		// NOTE(kp): Failed to initialize.
		return -1;
	}
	
	u64 window_flags =
		SDL_WINDOW_VULKAN |
		SDL_WINDOW_HIGH_PIXEL_DENSITY;
	
	window = SDL_CreateWindow(WINDOW_TITLE, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, window_flags);
	
	if(!window)
	{
		// NOTE(kp): Failed to create window.
		return -1;
	}
	
	// NOTE(kp): Init platform.
	{
		platform.permanent_memory_size = PERMANENT_MEMORY_SIZE;
		platform.frame_memory_size = FRAME_MEMORY_SIZE;
		platform.scratch_memory_size = SCRATCH_MEMORY_SIZE;
		
		platform.permanent_memory = malloc(platform.permanent_memory_size);
		platform.frame_memory = malloc(platform.frame_memory_size);
		platform.scratch_memory[0] = malloc(platform.scratch_memory_size);
		platform.scratch_memory[1] = malloc(platform.scratch_memory_size);
		
		platform.window_width = DEFAULT_WINDOW_WIDTH;
		platform.window_height = DEFAULT_WINDOW_HEIGHT;
		
		platform.window_opacity = 1.f;
		
		platform.fullscreen = 0;
		platform.borderless = 0;
		
		platform.target_fps = 120;
		platform.current_time = 0.f;
		
		platform.cursor_visible = 1;
		platform.cursor_locked = 0;
		
		platform.exit = 0;
		
		platform.GetVulkanInstanceExtensions = GetVulkanInstanceExtensions_SDL3;
		platform.CreateVulkanSurface = CreateVulkanSurface_SDL3;
		
		platform.GetTicks = SDL_GetTicks;
		platform.GetPerformanceCounter = SDL_GetPerformanceCounter;
		platform.GetPerformanceFrequency = SDL_GetPerformanceFrequency;
	}
	
	Init(&platform);
	
	while(!platform.exit)
	{
		SDL_Event ev = {};
		
		while(SDL_PollEvent(&ev))
		{
			f32 spx = 0.f, spy = 0.f;
			
			switch(ev.type)
			{
				case SDL_EVENT_QUIT:
				{
					platform.exit = 1;
				}
				break;
				
				case SDL_EVENT_WINDOW_RESIZED:
				{
				}
				break;
				
				case SDL_EVENT_KEY_DOWN:
				{
					platform.input.kb_down[ev.key.scancode] = 1;
					platform.input.kb_pressed[ev.key.scancode] = !prev_input_st.kb_down[ev.key.scancode];
				}
				break;
				
				case SDL_EVENT_KEY_UP:
				{
					platform.input.kb_down[ev.key.scancode] = 0;
					platform.input.kb_released[ev.key.scancode] = prev_input_st.kb_down[ev.key.scancode];
				}
				break;
				
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				{
					platform.input.mb_down[ev.button.button] = 1;
					platform.input.mb_pressed[ev.button.button] = !prev_input_st.mb_down[ev.button.button];
				}
				break;
				
				case SDL_EVENT_MOUSE_BUTTON_UP:
				{
					platform.input.mb_down[ev.button.button] = 0;
					platform.input.mb_released[ev.button.button] = prev_input_st.mb_down[ev.button.button];
				}
				break;
				
				case SDL_EVENT_MOUSE_MOTION:
				{
					SDL_GetGlobalMouseState(&spx, &spy);
					
					platform.input.mouse_position = v2(ev.motion.x, ev.motion.y);
					platform.input.mouse_delta = v2(ev.motion.xrel, ev.motion.yrel);
					platform.input.mouse_screen_position = v2(spx, spy);
				}
				break;
				
				case SDL_EVENT_MOUSE_WHEEL:
				{
					platform.input.mouse_wheel = v2(ev.wheel.x, ev.wheel.y);
				}
				break;
			}
		}
		
		Platform prev_st = platform;
		
		Update(&platform);
		
		if(platform.fullscreen != prev_st.fullscreen)
		{
			SDL_SetWindowFullscreen(window, platform.fullscreen);
		}
		
		if(platform.borderless != prev_st.borderless)
		{
			SDL_SetWindowBordered(window, platform.borderless);
		}
		
		if(platform.window_width != prev_st.window_width ||
		   platform.window_height != prev_st.window_height)
		{
			if(platform.window_width >= 0 && platform.window_height >= 0)
			{
				SDL_SetWindowSize(window, platform.window_width, platform.window_height);
			}
		}
		
		prev_input_st = platform.input;
	}
	
	Destroy(&platform);
	
	CloseAllGamepads();
	
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	free(platform.permanent_memory);
	free(platform.frame_memory);
	free(platform.scratch_memory[0]);
	free(platform.scratch_memory[1]);
	
	return 0;
}
