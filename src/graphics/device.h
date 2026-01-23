#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/types.h"

#include "container/vector.h"
#include "container/hash_map.h"
#include "container/string.h"

#include "bindless.h"
#include "command_pool.h"
#include "swapchain.h"
#include "command_pool.h"
#include "command_buffer.h"
#include "queue.h"
#include "gpu_buffer.h"
#include "texture.h"
#include "shader.h"
#include "pipeline.h"
#include "sampler.h"

#define GFX_VK_CHECK(fn, msg) \
	do { \
		VkResult _gfx_vk_check_result = (fn); \
		if (_gfx_vk_check_result != VK_SUCCESS) \
			debug_log_crash(msg " (%d)", _gfx_vk_check_result); \
	} while (0)

namespace gfx
{
	class Device {

	public:
		Device();
		~Device();

		void init();
		void destroy();

		void wait_idle();
		void wait_for_fence(VkFence fence);
		void reset_fence(VkFence fence);

		void destroy_fence(VkFence fence);
		void destroy_semaphore(VkSemaphore semaphore);

		// ---

		CommandBuffer begin_frame(Swapchain &swapchain);
		void end_frame(const Swapchain &swapchain, CommandBuffer &cmd);

		CommandBuffer begin_submit();
		void end_submit(CommandBuffer &cmd);

		// ---

		CommandPool create_command_pool(u32 family_index);
		void destroy_command_pool(const CommandPool &pool);
		void reset_command_pool(CommandPool &pool);

		// ---

		Swapchain create_swapchain();
		void destroy_swapchain(const Swapchain &swapchain);

		// ---

		VkPipelineLayout create_pipeline_layout(const ShaderProgram &program);
		VkPipelineLayout fetch_pipeline_layout(const ShaderProgram &program);
		void destroy_pipeline_layout(VkPipelineLayout layout);

		// ---

		VkPipeline create_pipeline(const GraphicsPipelineDef &def, VkPipelineLayout layout);
		VkPipeline create_pipeline(const ComputePipelineDef &def, VkPipelineLayout layout);

		PipelineState fetch_pipeline(const GraphicsPipelineDef &def);
		PipelineState fetch_pipeline(const ComputePipelineDef &def);

		void destroy_pipeline(VkPipeline pipeline);

		// ---

		Sampler create_sampler(
			VkFilter filter,
			VkSamplerAddressMode wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VkSamplerAddressMode wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VkSamplerAddressMode wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VkBorderColor border_colour = VK_BORDER_COLOR_INT_OPAQUE_BLACK
		);

		void destroy_sampler(const Sampler &sampler);

		// ---

		// TODO: The parameters here should be put into a struct like TextureAllocInfo.

		Texture alloc_texture(
			u32 width, u32 height, u32 depth,
			VkFormat format, VkImageType type, VkImageTiling tiling,
			u32 mipmaps, u32 layers,
			VkSampleCountFlags samples,
			bool is_transient, bool is_storage, bool is_cubemap
		);

		Texture alloc_texture_2d(u32 width, u32 height, VkFormat format, u32 mipmaps);
		Texture alloc_texture_2d_rw(u32 width, u32 height, VkFormat format, u32 mipmaps);
		Texture alloc_texture_2d_depth(u32 width, u32 height, u32 mipmaps);
		Texture alloc_texture_2d_rw_depth(u32 width, u32 height, u32 mipmaps);
		Texture alloc_texture_cubemap(u32 resolution, VkFormat format, u32 mipmaps);
		Texture alloc_texture_cubemap_depth(u32 resolution, u32 mipmaps);

		void destroy_texture(const Texture &texture);

		// ---

		TextureView create_texture_view(
			const Texture &texture,
			VkImageViewType type,
			u32 layer_count,
			u32 base_layer,
			u32 mipmap_count,
			u32 base_mip
		);

		TextureView fetch_texture_view(
			const Texture &texture,
			VkImageViewType type,
			u32 layer_count,
			u32 base_layer,
			u32 mipmap_count,
			u32 base_mip
		);

		TextureView fetch_texture_view_std(const Texture &texture);

		void destroy_texture_view(const TextureView &texture_view);

		// ---

		// TODO: The parameters here should be put into a struct like GPUBufferAllocInfo.

		GpuBuffer alloc_gpu_buffer(VkBufferUsageFlags2 usage, VmaAllocationCreateFlagBits flags, u64 size);
		void destroy_gpu_buffer(const GpuBuffer &gpu_buffer);

		// ---

		ShaderStage load_shader_stage_from_bytecode(const String &path);
		void destroy_shader_stage(const ShaderStage &stage);

		ShaderProgram create_shader_program(const Vector<String> &stage_paths);
		void destroy_shader_program(const ShaderProgram &program);

		// ---
		
		void init_imgui();
		void imgui_new_frame();
		void imgui_record_draw_data(const CommandBuffer &cmd);

		// ---

		const VkDevice &get_handle() const
		{
			return device;
		}

		u32 get_current_frame_index() const
		{
			return current_frame_index;
		}

		VkFormat get_depth_format() const
		{
			return depth_format;
		}

		VkSampleCountFlagBits get_max_sample_count() const
		{
			return max_msaa_samples;
		}

		const BindlessResources &get_bindless() const
		{
			return bindless;
		}

	private:
		void create_bindless();
		void destroy_bindless();
		void apply_bindless_updates();

		VkSemaphore get_current_render_finished_semaphore();
		VkSemaphore get_current_image_available_semaphore();
		
		void destroy_imgui();

		VkInstance instance;
		VkDevice device;

		VkPhysicalDevice physical_device;
		VkPhysicalDeviceProperties2 physical_device_properties;
		VkPhysicalDeviceFeatures2 physical_device_features;

		VmaAllocator vma_allocator;

		VkSurfaceKHR surface;
		VkPipelineCache pipeline_process_cache;

		VkDebugUtilsMessengerEXT debug_messenger;
		bool has_validation_layers;

		u32 current_frame_index;

		struct PerFrameData {
			VkFence in_flight_fence;
			VkSemaphore image_available_semaphore;
			VkSemaphore render_finished_semaphore;
		};

		PerFrameData per_frame_data[FRAMES_IN_FLIGHT];

		Queue graphics_queue;

		VkFormat depth_format;
		VkSampleCountFlagBits max_msaa_samples;

		SwapchainSupportDetails swapchain_details;

		// Bindless is built into the device.
		// TODO: Should this be moved out?
		BindlessResources bindless;

		HashMap<u64, TextureView> texture_view_cache;
		HashMap<u64, VkPipeline> pipeline_cache;
		HashMap<u64, VkPipelineLayout> pipeline_layout_cache;

		VkDescriptorPool imgui_pool;
	};
}
