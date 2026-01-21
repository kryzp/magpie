#pragma once

#include "core/types.h"
#include "math/vec2.h"

#include "input.h"

#define DEFAULT_WINDOW_TITLE    "Demo"
#define ENGINE_NAME             "Magpie"

#define DEFAULT_WINDOW_WIDTH     1280
#define DEFAULT_WINDOW_HEIGHT    720

#define APP_VERSION_VARIANT      0
#define APP_VERSION_MAJOR        0
#define APP_VERSION_MINOR        1
#define APP_VERSION_PATCH        0

#define ENGINE_VERSION_VARIANT   1
#define ENGINE_VERSION_MAJOR     0
#define ENGINE_VERSION_MINOR     1
#define ENGINE_VERSION_PATCH     0

#define SCRATCH_MEMORY_SIZE      MEGABYTES(2)

typedef void (*fiber_entry_point_fn)(void *param);

struct Platform {
	const char *window_title;

	u32 window_width;
	u32 window_height;

	int window_pixel_width;
	int window_pixel_height;

	float window_opacity;

	bool fullscreen;
	bool borderless;

	float target_fps;
	float current_time;

	bool cursor_visible;
	bool cursor_locked;

	void (*set_window_size)(u32 width, u32 height);
	void (*set_window_fullscreen)(bool b);
	void (*set_window_borderless)(bool b);

	void (*set_mouse_position)(u32 x, u32 y);

	u64 (*get_ticks)(void);
	u64 (*get_performance_counter)(void);
	u64 (*get_performance_frequency)(void);

	void *(*create_thread)(ulong (*entry)(void *param), void *param);
	void (*join_thread)(void *handle);
	void (*detach_thread)(void *handle);

	void *(*convert_thread_to_fiber)(void);
	int (*convert_fiber_to_thread)(void);
	void *(*create_fiber)(u32 stack_size, fiber_entry_point_fn entry, void *param);
	void (*delete_fiber)(void *handle);
	void (*switch_to_fiber)(void *handle);

	bool (*file_delete)(const char *path);
	bool (*file_exists)(const char *path);

	bool (*dir_create)(const char *path);
	bool (*dir_delete)(const char *path);
	bool (*dir_exists)(const char *path);

	void *(*stream_from_file)(const char *path, const char *mode);
	void *(*stream_from_memory)(void *data, u64 size);
	void *(*stream_from_const_memory)(const void *data, u64 size);
	s64 (*stream_read)(void *stream, void *dst, u64 size);
	s64 (*stream_write)(void *stream, const void *src, u64 size);
	s64 (*stream_seek)(void *stream, s64 offset);
	s64 (*stream_size)(void *stream);
	s64 (*stream_position)(void *stream);
	bool (*stream_close)(void *stream);

	void (*open_in_explorer)(const char *path);

	bool (*create_vulkan_surface)(void *instance, void *surface_pointer);
	void (*destroy_vulkan_surface)(void *instance, void *surface);
	const char *const *(*get_vulkan_instance_extensions)(u32 *count);
};
