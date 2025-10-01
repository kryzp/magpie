#ifndef GFX_COMMAND_BUFFER_H
#define GFX_COMMAND_BUFFER_H

#include <volk/volk.h>

#include "texture.h"
#include "buffer.h"
#include "pipeline.h"

struct gfx_device;

struct gfx_command_buffer {
	VkCommandBuffer handle;
};

void gfx_cmd_begin(struct gfx_command_buffer *cmd);
void gfx_cmd_end(struct gfx_command_buffer *cmd);

void gfx_cmd_set_viewport(struct gfx_command_buffer *cmd, VkViewport viewport);
void gfx_cmd_set_scissor(struct gfx_command_buffer *cmd, VkRect2D scissor);

void gfx_cmd_begin_rendering(struct gfx_command_buffer *cmd, struct gfx_render_info *info);
void gfx_cmd_end_rendering(struct gfx_command_buffer *cmd);

void gfx_cmd_pipeline_barrier(struct gfx_command_buffer *cmd,
			      VkDependencyFlags dependency_flags,
			      u32 memory_barrier_count, VkMemoryBarrier2 *memory_barriers,
			      u32 buffer_memory_barrier_count, VkBufferMemoryBarrier2 *buffer_memory_barriers,
			      u32 image_memory_barrier_count, VkImageMemoryBarrier2 *image_memory_barriers);

void gfx_cmd_bind_descriptors(struct gfx_command_buffer *cmd,
			      VkPipelineBindPoint bind_point,
			      VkPipelineLayout layout, u32 first,
			      u32 descriptor_count, VkDescriptorSet *descriptors);

void gfx_cmd_bind_descriptors_dyoff(struct gfx_command_buffer *cmd,
				    VkPipelineBindPoint bind_point,
				    VkPipelineLayout layout, u32 first,
				    u32 descriptor_count, VkDescriptorSet *descriptors,
				    u32 dynamic_offset_count, u32 *dynamic_offsets);

void gfx_cmd_bind_bindless(struct gfx_command_buffer *cmd,
			   VkPipelineBindPoint bind_point,
			   VkPipelineLayout layout,
			   struct gfx_device *device);

void gfx_cmd_bind_pipeline(struct gfx_command_buffer *cmd,
			   VkPipelineBindPoint bind_point,
			   VkPipeline pipeline);

void gfx_cmd_bind_index_buffer(struct gfx_command_buffer *cmd,
			       struct gfx_buffer *buffer,
			       u64 offset);

void gfx_cmd_push_constants(struct gfx_command_buffer *cmd,
			    VkPipelineLayout layout,
			    VkShaderStageFlags stage_flags,
			    u32 size, void *data);

void gfx_cmd_push_constants_offset(struct gfx_command_buffer *cmd,
				   VkPipelineLayout layout,
				   VkShaderStageFlags stage_flags,
				   u32 size, void *data, u32 offset);

void gfx_cmd_draw_vertices_n(struct gfx_command_buffer *cmd,
			     u32 vertex_count);

void gfx_cmd_draw_indexed(struct gfx_command_buffer *cmd,
			  u32 index_count,
			  u32 instance_count,
			  u32 first_index,
			  s32 vertex_offset,
			  u32 first_instance);

void gfx_cmd_draw_indexed_indirect(struct gfx_command_buffer *cmd,
				   struct gfx_buffer *buffer,
				   u64 offset,
				   u32 count,
				   u32 stride);

void gfx_cmd_blit(struct gfx_command_buffer *cmd,
		  struct gfx_texture *src,
		  struct gfx_texture *dst,
		  u32 region_count, VkImageBlit2 *regions,
		  VkFilter filter);

void gfx_cmd_generate_mipmaps(struct gfx_command_buffer *cmd,
			      struct gfx_texture *texture);

void gfx_cmd_copy_buffer_to_buffer(struct gfx_command_buffer *cmd,
				   struct gfx_buffer *src,
				   struct gfx_buffer *dst,
				   u32 region_count, VkBufferCopy *regions);

void gfx_cmd_copy_buffer_to_texture_multi_region(struct gfx_command_buffer *cmd,
						 struct gfx_buffer *buffer,
						 struct gfx_texture *texture,
						 u32 region_count, VkBufferImageCopy *regions);

void gfx_cmd_copy_buffer_to_texture(struct gfx_command_buffer *cmd,
				    struct gfx_buffer *buffer,
				    struct gfx_texture *texture);

void gfx_cmd_dispatch(struct gfx_command_buffer *cmd,
		      u32 x, u32 y, u32 z);

#endif // GFX_COMMAND_BUFFER_H
