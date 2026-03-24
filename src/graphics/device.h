#pragma once

#include <functional>

#include "core/types.h"

#include "container/vector.h"

#include "context.h"
#include "bindless.h"
#include "command_pool.h"
#include "swapchain.h"
#include "command_buffer.h"
#include "gpu_buffer.h"
#include "texture.h"
#include "shader.h"
#include "pipeline.h"
#include "sampler.h"

namespace gfx
{
	class Context;

	struct TextureAllocInfo {
		u32 width, height, depth;
		VkFormat format;
		VkImageType type;
		VkImageTiling tiling;
		u32 mipmaps;
		u32 layers;
		VkSampleCountFlags samples;
		bool is_transient;
		bool is_storage;
		bool is_cubemap;
	};

	struct BufferAllocInfo {
		VkBufferUsageFlags2 usage;
		VmaAllocationCreateFlags flags;
		u64 size;
	};

	struct SamplerCreateInfo {
		VkFilter filter;
		VkSamplerAddressMode wrap_x;
		VkSamplerAddressMode wrap_y;
		VkSamplerAddressMode wrap_z;
		VkBorderColor border_colour;
	};

	class Device {
	public:
		Device();
		~Device();

		void init();
		void destroy();

		// ---

		void wait_idle();

		void wait_for_fence(VkFence fence) const;
		void reset_fence(VkFence fence) const;
		void destroy_fence(VkFence fence) const;

		VkSemaphore create_timeline_semaphore(u64 initial_value) const;
		void wait_for_timeline_semaphore(VkSemaphore semaphore, u64 value) const;
		u64 get_timeline_semaphore_value(VkSemaphore semaphore) const;
		void destroy_semaphore(VkSemaphore semaphore) const;

		// ---

		void destroy_query_pool(VkQueryPool pool) const;

		// ---

		CommandBuffer begin_frame(Swapchain &swapchain);
		void end_frame(const Swapchain &swapchain, CommandBuffer &cmd);

		// ---

		u64 submit_graphics(
			CommandBuffer &cmd,
			const Vector<VkSemaphoreSubmitInfo> &waits,
			const Vector<VkSemaphoreSubmitInfo> &signals,
			VkFence fence
		);

		void submit_graphics_immediate(
			const std::function<void(CommandBuffer &cmd)> &record
		);
		
		void present(
			const Swapchain &swapchain,
			const Vector<VkSemaphore> &waits
		);

		// ---

		CommandPool create_command_pool(u32 family_index);
		void destroy_command_pool(const CommandPool &pool);
		void reset_command_pool(CommandPool &pool);
		CommandBuffer fetch_free_buffer(CommandPool &pool);

		// ---

		Swapchain create_swapchain();
		void destroy_swapchain(const Swapchain &swapchain);

		// ---
		
		VkPipelineLayout create_pipeline_layout(const ShaderProgram *program);
		void destroy_pipeline_layout(VkPipelineLayout layout);

		VkPipeline create_pipeline(const GraphicsPipelineDef &def, VkPipelineLayout layout);
		VkPipeline create_pipeline(const ComputePipelineDef &def, VkPipelineLayout layout);

		void destroy_pipeline(VkPipeline pipeline);

		// ---

		Sampler *create_sampler(const SamplerCreateInfo &info);
		Sampler *create_sampler(VkFilter filter);

		void destroy_sampler(const Sampler *sampler);

		// ---

		// TODO: The parameters here should be put into a struct like TextureAllocInfo.

		Texture *alloc_texture(const TextureAllocInfo &alloc_info);

		Texture *alloc_texture_2d(u32 width, u32 height, VkFormat format, u32 mipmaps);
		Texture *alloc_texture_2d_rw(u32 width, u32 height, VkFormat format, u32 mipmaps);
		Texture *alloc_texture_2d_depth(u32 width, u32 height, u32 mipmaps);
		Texture *alloc_texture_2d_rw_depth(u32 width, u32 height, u32 mipmaps);
		Texture *alloc_texture_cubemap(u32 resolution, VkFormat format, u32 mipmaps);
		Texture *alloc_texture_cubemap_depth(u32 resolution, u32 mipmaps);

		void destroy_texture(const Texture *texture);

		TextureView *create_texture_view(
			const Texture *texture,
			VkImageViewType type,
			const SubresourceRange &range
		);

		void destroy_texture_view(const TextureView *texture_view);

		// ---

		// TODO: The parameters here should be put into a struct like GPUBufferAllocInfo.

		GpuBuffer *alloc_buffer(const BufferAllocInfo &alloc_info);
		GpuBuffer *alloc_stage(u64 size);

		void destroy_buffer(const GpuBuffer *gpu_buffer);

		// ---

		ShaderProgram *create_shader_program(const Vector<ShaderBytecode> &stages);
		void destroy_shader_program(const ShaderProgram *program);

		// ---
		
		void imgui_new_frame();
		void imgui_record_draw_data(const CommandBuffer &cmd);

		// ---

		u32 get_current_frame_index() const
		{
			return current_frame_index;
		}

		u64 get_graphics_timeline_value() const
		{
			return graphics_timeline_value;
		}

		u64 get_graphics_completed_timeline_value() const
		{
			return get_timeline_semaphore_value(graphics_timeline_semaphore);
		}

		// ---

		const Context &get_context() const
		{
			return context;
		}

		BindlessResources &get_bindless()
		{
			return bindless;
		}

	private:
		void create_sync_resources();
		void destroy_sync_resources();

		void create_bindless();
		void destroy_bindless();
		void apply_bindless_updates();
		
		void init_imgui();
		void destroy_imgui();

		Context context;
		
		u32 current_frame_index;
		
		VkPipelineCache pipeline_process_cache;

		VkSemaphore graphics_timeline_semaphore;
		u64 graphics_timeline_value;

		struct PerFrameData {
			u64 expected_timeline_value;

			VkSemaphore image_available_semaphore; // Wait until OS gives us an image.
			VkSemaphore render_finished_semaphore; // Signaled when OS allows us to present.

			CommandPool command_pool;

			struct ImageDestroy {
				VkImage handle;
				VmaAllocation allocation;
			};

			struct BufferDestroy {
				VkBuffer handle;
				VmaAllocation allocation;
			};

			Vector<VkSampler> destroyed_samplers;
			Vector<ImageDestroy> destroyed_images;
			Vector<VkImageView> destroyed_views;
			Vector<BufferDestroy> destroyed_buffers;
			Vector<BindlessHandle> destroyed_bindless_samplers;
			Vector<BindlessHandle> destroyed_bindless_views;

			void flush(VkDevice vk_device, VmaAllocator vma_allocator, BindlessResources &bindless);
		};

		PerFrameData per_frame_data[FRAMES_IN_FLIGHT];

		BindlessResources bindless;

		VkDescriptorPool imgui_pool;
	};
}
