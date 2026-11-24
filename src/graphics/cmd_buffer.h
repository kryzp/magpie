#ifndef GRAPHICS_CMD_BUFFER_H
#define GRAPHICS_CMD_BUFFER_H

#include "core/types.h"
#include "core/vec4.h"
#include "graphics/resource_key.h"
#include "graphics/mesh.h"
#include "graphics/accel_struct.h"
#include "graphics/pipeline.h"

#include <volk/volk.h>

namespace gfx {

struct RenderInfo;

struct BufferCopy {
	u64 src_offset = 0;
	u64 dst_offset = 0;
	u64 size = 0;
};

struct BufferImageCopy {
	u64 buffer_offset = 0;
	unsigned buffer_row_length = 0;
	unsigned buffer_image_height = 0;

	VkImageSubresourceLayers image_subresource = {};

	int x = 0;
	int y = 0;
	int z = 0;

	unsigned width = 0;
	unsigned height = 0;
	unsigned depth = 0;
};

class CmdBuffer {
public:
	CmdBuffer()
		: vk_handle(VK_NULL_HANDLE)
	{
	}

	explicit CmdBuffer(VkCommandBuffer handle)
		: vk_handle(handle)
	{
	}

	[[nodiscard]] VkCommandBuffer GetVkHandle() const
	{
		return vk_handle;
	}


	/* ==================================================
	   CORE
	   ================================================== */

	void Begin() const;
	void End() const;


	/* ==================================================
	   DYNAMIC RENDERING
	   ================================================== */

	void BeginRendering(const RenderInfo &info) const;
	void EndRendering() const;

	void SetViewport(VkViewport viewport) const;
	void SetScissor(VkRect2D scissor) const;


	/* ==================================================
	   SYNC
	   ================================================== */

	void PipelineBarrier(
		VkDependencyFlags dependency_flags,
		unsigned memory_barrier_count, const VkMemoryBarrier2 *memory_barriers,
		unsigned buffer_barrier_count, const VkBufferMemoryBarrier2 *buffer_barriers,
		unsigned image_barrier_count, const VkImageMemoryBarrier2 *image_barriers
	) const;


	/* ==================================================
	   DESCRIPTORS
	   ================================================== */

	void BindDescriptors(
		VkShaderStageFlags stage_flags,
		ResourceKey layout, unsigned first,
		unsigned descriptor_count, const VkDescriptorSet *descriptors,
		unsigned dynamic_offset_count, const unsigned *dynamic_offsets
	) const;

	void BindBindless(VkShaderStageFlags stage_flags, ResourceKey layout) const;


	/* ==================================================
	   PIPELINE
	   ================================================== */

	void BindPipeline(VkPipelineBindPoint bind_point, ResourceKey pipeline) const;

	// also binds the bindless set implicitly on all graphics / compute
	void BindPipelineState(const PipelineState &state) const;


	/* ==================================================
	   INDEX BUFFER
	   ================================================== */

	void BindIndexBuffer(
		ResourceKey buffer,
		u64 offset, u64 size, // VK_WHOLE_SIZE
		IndexType index_type
	) const;


	/* ==================================================
	   PUSH CONSTANTS
	   ================================================== */

	void PushConstantsEx(
		ResourceKey layout,
		VkShaderStageFlags stage_flags,
		u64 size, const void *data,
		unsigned offset
	) const;

	template <typename T>
	void PushConstants(ResourceKey layout, VkShaderStageFlags stage_flags, const T &value, unsigned offset = 0) const
	{
		PushConstantsEx(layout, stage_flags, sizeof(T), &value, offset);
	}


	/* ==================================================
	   DRAW
	   ================================================== */

	void SetLineWidth(float thickness) const;

	void Draw(
		unsigned vertex_count,
		unsigned instance_count = 1,
		unsigned first_vertex = 0,
		unsigned first_instance = 0
	) const;

	void DrawIndexed(
		unsigned index_count,
		unsigned instance_count,
		unsigned first_index,
		int vertex_offset,
		unsigned first_instance
	) const;

	void DrawIndexedIndirect(
		ResourceKey buffer, u64 offset,
		unsigned count, unsigned stride
	) const;

	void DrawIndexedIndirectCount(
		ResourceKey indirect_buffer, u64 indirect_offset,
		ResourceKey count_buffer, u64 count_offset,
		unsigned max_count, unsigned stride
	) const;

	void DrawMeshTasksIndirectCount(
		ResourceKey indirect_buffer, u64 indirect_offset,
		ResourceKey count_buffer, u64 count_offset,
		unsigned max_count, unsigned stride
	) const;


	/* ==================================================
	   DISPATCH
	   ================================================== */

	void Dispatch(unsigned x, unsigned y, unsigned z) const;

	void DispatchIndirect(ResourceKey buffer, u64 offset) const;


	/* ==================================================
	   BLIT
	   ================================================== */

	void Blit(
		ResourceKey src_texture,
		ResourceKey dst_texture,
		unsigned region_count, const VkImageBlit2 *regions,
		VkFilter filter
	) const;

	void GenerateMipmaps(ResourceKey texture) const;


	/* ==================================================
	   TRANSFER
	   ================================================== */

	void CopyBufferToBuffer(
		ResourceKey src_buffer,
		ResourceKey dst_buffer,
		unsigned region_count, const BufferCopy *regions
	) const;

	void CopyBufferToTexture(
		ResourceKey src_buffer,
		ResourceKey dst_texture,
		unsigned region_count,
		const BufferImageCopy *regions
	) const;

	void CopyTextureToBuffer(
		ResourceKey src_texture,
		ResourceKey dst_buffer,
		unsigned region_count,
		const BufferImageCopy *regions
	) const;

	void CopyBufferToTextureWhole(
		ResourceKey src_buffer,
		ResourceKey dst_texture,
		u64 buffer_offset
	) const;

	void FillBuffer(
		ResourceKey buffer,
		u64 offset,
		u64 size,
		unsigned fill
	) const;


	/* ==================================================
	   ACCELERATION STRUCTURES
	   ================================================== */

	void BuildBLAS(
		ResourceKey blas,
		const BLASGeometry *geometries,
		unsigned geometry_count,
		ResourceKey scratch_buffer
	) const;

	void BuildTLAS(
		ResourceKey tlas,
		ResourceKey instance_buffer,
		unsigned instance_count,
		ResourceKey scratch_buffer
	) const;


	/* ==================================================
	   QUERY / PROFILING
	   ================================================== */

	void BeginQuery(VkQueryPool pool, unsigned query, VkQueryControlFlags flags) const;
	void EndQuery(VkQueryPool pool, unsigned query) const;
	void ResetQueries(VkQueryPool pool, unsigned first, unsigned count) const;
	void WriteTimestamp(VkPipelineStageFlags2 stage, VkQueryPool pool, unsigned index) const;

	void BeginLabel(const char *name) const;
	void BeginLabel(const char *name, const Vec4 &colour) const;
	void EndLabel() const;

private:
	VkCommandBuffer vk_handle;
};

}

#endif // GRAPHICS_CMD_BUFFER_H
