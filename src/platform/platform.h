#pragma once

#include "core/types.h"

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

namespace platform
{
	void set_window_title(const char *title);
	void get_window_size(int *width, int *height);
	void get_window_size_in_pixels(int *pixel_width, int *pixel_height);
	void set_window_size(u32 width, u32 height);
	void set_window_fullscreen(bool b);
	void set_window_borderless(bool b);
	void set_window_opacity(float opacity);

	void set_mouse_position(u32 x, u32 y);
	void set_mouse_visible(bool visible);
	void set_mouse_locked(bool locked);

	u64 get_ticks();
	u64 get_performance_counter();
	u64 get_performance_frequency();

	void *create_thread(ulong (*entry)(void *param), void *param);
	void join_thread(void *handle);
	void detach_thread(void *handle);

	void *convert_thread_to_fiber();
	int convert_fiber_to_thread();
	void *create_fiber(u32 stack_size, fiber_entry_point_fn entry, void *param);
	void delete_fiber(void *handle);
	void switch_to_fiber(void *handle);

	bool file_delete(const char *path);
	bool file_exists(const char *path);

	bool dir_create(const char *path);
	bool dir_delete(const char *path);
	bool dir_exists(const char *path);

	void *stream_from_file(const char *path, const char *mode);
	void *stream_from_memory(void *data, u64 size);
	void *stream_from_const_memory(const void *data, u64 size);
	s64 stream_read(void *stream, void *dst, u64 size);
	s64 stream_write(void *stream, const void *src, u64 size);
	s64 stream_seek(void *stream, s64 offset);
	s64 stream_size(void *stream);
	s64 stream_position(void *stream);
	bool stream_close(void *stream);

	void open_in_explorer(const char *path);

	bool create_vulkan_surface(void *instance, void *surface_pointer);
	void destroy_vulkan_surface(void *instance, void *surface);
	const char *const *get_vulkan_instance_extensions(u32 *count);
}
