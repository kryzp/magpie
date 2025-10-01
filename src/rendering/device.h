#ifndef GFX_DEVICE_H
#define GFX_DEVICE_H

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/core_types.h"
#include "core/core_math.h"
#include "core/core_string.h"

#include "data/hash_table.h"

#include "platform/platform.h"

#include "sampler.h"
#include "texture.h"
#include "buffer.h"
#include "shader.h"
#include "swapchain.h"
#include "pipeline.h"
#include "command_buffer.h"
#include "command_pool.h"
#include "queue.h"
#include "bindless.h"

#define GFX_FRAMES_IN_FLIGHT 3

#define GFX_VK_CHECK(func_call, error_msg)				\
	do {								\
		VkResult _gfx_vk_check_result = func_call;		\
		if (_gfx_vk_check_result != VK_SUCCESS)			\
			debug_log_crash(error_msg " (%d)", _gfx_vk_check_result); \
	} while (0)

struct gfx_sync_data {
	struct gfx_command_pool command_pool;
	
	VkFence in_flight_fence;
	VkFence instant_submit_fence;

	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
};

struct gfx_device {
	struct memory_arena *arena;
	
	VkInstance instance;
	VkDevice device;

	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties2 physical_device_properties;
	VkPhysicalDeviceFeatures2 physical_device_features;

	VmaAllocator vma_allocator;

	VkDebugUtilsMessengerEXT debug_messenger;
	bool has_validation_layers;

	u32 current_frame_index;
	struct gfx_sync_data frames[GFX_FRAMES_IN_FLIGHT];

	struct gfx_queue graphics_queue;
	
	VkSurfaceKHR surface;
	VkPipelineCache pipeline_process_cache;

	VkFormat depth_format;
	VkSampleCountFlagBits max_msaa_samples;

	struct gfx_swapchain_support_details swapchain_details;
	
	// Bindless is built into the device.
	// TODO: Should this be moved out?
	struct gfx_bindless bindless;
	
	struct hash_table texture_view_cache;
	struct hash_table pipeline_cache;
	struct hash_table pipeline_layout_cache;
};

void gfx_device_init(struct gfx_device *device, struct platform *platform, struct memory_arena *arena);
void gfx_device_destroy(struct gfx_device *device, struct platform *platform);

void gfx_device_hot_load(struct gfx_device *device);
void gfx_device_hot_unload(struct gfx_device *device);

VkSemaphore gfx_device_current_render_finished_semaphore(struct gfx_device *device);
VkSemaphore gfx_device_current_image_available_semaphore(struct gfx_device *device);

void gfx_device_wait_idle(struct gfx_device *device);
void gfx_device_wait_for_fence(struct gfx_device *device, VkFence fence);
void gfx_device_reset_fence(struct gfx_device *device, VkFence fence);

// ---

struct gfx_command_buffer gfx_device_begin_present(struct gfx_device *device, struct gfx_swapchain *swapchain);
void gfx_device_end_present(struct gfx_device *device, struct gfx_swapchain *swapchain, struct gfx_command_buffer *cmd);

struct gfx_command_buffer gfx_device_begin_instant_submit(struct gfx_device *device);
void gfx_device_end_instant_submit(struct gfx_device *device, struct gfx_command_buffer *cmd);

// ---

struct gfx_swapchain gfx_device_swapchain_create(struct gfx_device *device, struct platform *platform);
void gfx_device_swapchain_destroy(struct gfx_device *device, struct gfx_swapchain *swapchain);
void gfx_device_swapchain_acquire_next_image(struct gfx_device *device, struct gfx_swapchain *swapchain);

// ---

struct gfx_bindless gfx_device_bindless_create(struct gfx_device *device);
void gfx_device_bindless_destroy(struct gfx_device *device, struct gfx_bindless *bindless);
void gfx_device_bindless_apply_updates(struct gfx_device *device, struct gfx_bindless *bindless);

// ---

VkPipelineLayout gfx_device_pipeline_layout_create(struct gfx_device *device, struct gfx_shader_program *program);
void gfx_device_pipeline_layout_destroy(struct gfx_device *device, VkPipelineLayout layout);

VkPipelineLayout gfx_device_pipeline_layout_fetch(struct gfx_device *device, struct gfx_shader_program *program);

// ---

VkPipeline gfx_device_pipeline_create_graphics(struct gfx_device *device, VkPipelineLayout layout, struct gfx_graphics_pipeline_def *def);
VkPipeline gfx_device_pipeline_create_compute(struct gfx_device *device, VkPipelineLayout layout, struct gfx_compute_pipeline_def *def);

void gfx_device_pipeline_destroy(struct gfx_device *device, VkPipeline pipeline);

// ---

struct gfx_pipeline_st gfx_device_pipeline_fetch_graphics(struct gfx_device *device, struct gfx_graphics_pipeline_def *definition);
struct gfx_pipeline_st gfx_device_pipeline_fetch_compute(struct gfx_device *device, struct gfx_compute_pipeline_def *definition);

// ---

struct gfx_sampler gfx_device_sampler_create_ext(struct gfx_device *device,
						 VkFilter filter,
						 VkSamplerAddressMode wrap_x,
						 VkSamplerAddressMode wrap_y,
						 VkSamplerAddressMode wrap_z,
						 VkBorderColor border_colour);

struct gfx_sampler gfx_device_sampler_create(struct gfx_device *device, VkFilter filter);

void gfx_device_sampler_destroy(struct gfx_device *device, struct gfx_sampler *sampler);

// ---

struct gfx_texture gfx_device_texture_alloc(struct gfx_device *device,
					    u32 width, u32 height, u32 depth,
					    VkFormat format,
					    VkImageViewType type,
					    VkImageTiling tiling,
					    u32 mipmaps,
					    VkSampleCountFlagBits samples,
					    bool is_transient, bool is_storage);

struct gfx_texture gfx_device_texture_alloc_2d(struct gfx_device *device,
					       u32 width, u32 height,
					       VkFormat format,
					       u32 mipmaps);

struct gfx_texture gfx_device_texture_alloc_2d_rw(struct gfx_device *device,
						  u32 width, u32 height,
						  VkFormat format,
						  u32 mipmaps);

struct gfx_texture gfx_device_texture_alloc_depth_2d(struct gfx_device *device,
						     u32 width, u32 height,
						     u32 mipmaps);

struct gfx_texture gfx_device_texture_alloc_depth_rw_2d(struct gfx_device *device,
							u32 width, u32 height,
							u32 mipmaps);

struct gfx_texture gfx_device_texture_alloc_cubemap(struct gfx_device *device,
						    u32 resolution,
						    VkFormat format,
						    u32 mipmaps);

void gfx_device_texture_destroy(struct gfx_device *device, struct gfx_texture *texture);

struct gfx_texture_view gfx_device_texture_view_create(struct gfx_device *device,
						       struct gfx_texture *texture,
						       u32 layer_count,
						       u32 base_layer,
						       u32 base_level);

void gfx_device_texture_view_destroy(struct gfx_device *device, struct gfx_texture_view *view);

struct gfx_texture_view *gfx_device_texture_view_fetch(struct gfx_device *device,
						       struct gfx_texture *texture,
						       u32 layer_count,
						       u32 base_layer,
						       u32 base_level);

struct gfx_texture_view *gfx_device_texture_view_fetch_std(struct gfx_device *device,
							   struct gfx_texture *texture);

// ---

struct gfx_buffer gfx_device_buffer_alloc(struct gfx_device *device,
					  VkBufferUsageFlags2 usage,
					  VmaAllocationCreateFlagBits flags,
					  u64 size);

void gfx_device_buffer_destroy(struct gfx_device *device, struct gfx_buffer *buffer);

// ---

struct gfx_shader_stage gfx_device_shader_stage_load_from_bytecode(struct gfx_device *device,
								   struct string8 path);

void gfx_device_shader_stage_destroy(struct gfx_device *device, struct gfx_shader_stage *stage);

// ---

struct gfx_shader_program gfx_device_shader_program_create(struct gfx_device *device,
							   u32 stage_count,
							   struct string8 *stage_paths);

void gfx_device_shader_program_destroy(struct gfx_device *device, struct gfx_shader_program *program);

// ---

struct gfx_command_pool gfx_device_command_pool_create(struct gfx_device *device, u32 family_index);
void gfx_device_command_pool_destroy(struct gfx_device *device, struct gfx_command_pool *pool);
void gfx_device_command_pool_reset(struct gfx_device *device, struct gfx_command_pool *pool);

#endif // GFX_DEVICE_H
