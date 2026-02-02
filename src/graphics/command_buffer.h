#pragma once

#include <volk/volk.h>

#include "core/types.h"
#include "container/vector.h"

#include "pipeline.h"
#include "bindless.h"
#include "gpu_buffer.h"
#include "texture.h"
#include "texture.h"

namespace gfx
{
	class CommandBuffer {
	public:
		CommandBuffer(VkCommandBuffer handle);
		~CommandBuffer();

		const VkCommandBuffer &get_handle() const
		{
			return handle;
		}

		// ---

		void begin();
		void end();

		void begin_rendering(const RenderInfo &info);
		void end_rendering();

		// ---

		void set_viewport(VkViewport viewport);
		void set_scissor(VkRect2D scissor);

		// ---

		void pipeline_barrier(
			VkDependencyFlags dependency_flags,
			const Vector<VkMemoryBarrier2> &memory_barriers,
			const Vector<VkBufferMemoryBarrier2> &buffer_memory_barriers,
			const Vector<VkImageMemoryBarrier2> &image_memory_barriers
		);

		// ---

		void bind_descriptors(
			VkPipelineBindPoint bind_point,
			VkPipelineLayout layout, u32 first,
			const Vector<VkDescriptorSet> &descriptors,
			const Vector<u32> &dynamic_offsets
		);

		void bind_bindless(
			VkPipelineBindPoint bind_point,
			VkPipelineLayout layout,
			const BindlessResources &bindless
		);

		void bind_pipeline(
			VkPipelineBindPoint bind_point,
			VkPipeline pipeline
		);

		void bind_index_buffer(
			const GpuBuffer *buffer,
			u64 offset
		);

		void push_constants(
			VkPipelineLayout layout,
			VkShaderStageFlags stage_flags,
			u64 size, void *data,
			u32 offset = 0
		);

		// ---

		void draw_vertices_n(u64 count);

		void draw_indexed(
			u32 index_count,
			u32 instance_count,
			u32 first_index,
			s32 vertex_offset,
			u32 first_instance
		);

		void draw_indexed_indirect(
			const GpuBuffer *buffer, u64 offset,
			u32 count, u32 stride
		);

		void draw_indexed_indirect_count(
			const GpuBuffer *buffer, u64 offset,
			const GpuBuffer *count_buffer, u64 count_offset,
			u32 count, u32 stride
		);

		// ---

		void blit(
			const Texture *src,
			const Texture *dst,
			const Vector<VkImageBlit2> &regions,
			VkFilter filter
		);

		void generate_mipmaps(const Texture *texture);

		// ---

		void copy_buffer_to_buffer(
			const GpuBuffer *src,
			const GpuBuffer *dst,
			const Vector<VkBufferCopy> &regions
		);

		void copy_entire_buffer_to_texture(
			const GpuBuffer *src,
			const Texture *dst
		);

		void copy_buffer_to_texture(
			const GpuBuffer *src,
			const Texture *dst,
			const Vector<VkBufferImageCopy> &regions
		);

		// ---

		void fill_buffer(
			const GpuBuffer *buffer,
			u64 offset, u64 size, u32 data
		);

		// ---

		void dispatch(u32 x, u32 y, u32 z);

	private:
		VkCommandBuffer handle;
	};
}
