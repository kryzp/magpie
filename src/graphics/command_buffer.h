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
	inline u32 compute_group_count(u32 count, u32 tile)
	{
		return (count + tile - 1) / tile;
	}

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
			VkShaderStageFlags stage_flags,
			VkPipelineLayout layout, u32 first,
			const Vector<VkDescriptorSet> &descriptors,
			const Vector<u32> &dynamic_offsets
		);

		void bind_bindless(
			VkShaderStageFlags stage_flags,
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
			u64 size, const void *data,
			u32 offset = 0
		);

		// ---

		void set_line_width(float thickness);

		// ---

		void draw(
			u32 vertex_count,
			u32 instance_count = 1,
			u32 first_vertex = 0,
			u32 first_instance = 0
		);

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
			u32 max_count, u32 stride
		);

		void draw_mesh_tasks_indirect_count(
			const GpuBuffer *buffer, u64 offset,
			const GpuBuffer *count_buffer, u64 count_offset,
			u32 max_count, u32 stride
		);

		// ---

		void dispatch(u32 x, u32 y, u32 z);

		void dispatch_indirect(const GpuBuffer *buffer, u64 offset);

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
			const Vector<VkBufferCopy2> &regions
		);

		void copy_buffer_to_texture(
			const GpuBuffer *src,
			const Texture *dst,
			u64 buffer_offset = 0
		);

		void copy_buffer_to_texture_region(
			const GpuBuffer *src,
			const Texture *dst,
			const Vector<VkBufferImageCopy2> &regions
		);

		// ---

		void fill_buffer(
			const GpuBuffer *buffer,
			u64 offset, u64 size, u32 data
		);

		// ---

		void begin_query(VkQueryPool pool, u32 query, VkQueryControlFlags flags);
		void end_query(VkQueryPool pool, u32 query);

		void reset_queries(VkQueryPool pool, u32 first, u32 count);

		void write_timestamp(VkPipelineStageFlags2 stage, VkQueryPool pool, u32 index);

	private:
		VkCommandBuffer handle;
	};
}
