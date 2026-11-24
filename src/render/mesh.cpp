#include "render/mesh.h"
#include "graphics/device.h"

void rnd::Mesh::Alloc(
	u64 vertex_stride_,
	gfx::IndexType index_type_,
	unsigned vertex_count_,
	unsigned index_count_
)
{
	vertex_stride = vertex_stride_;
	index_type = index_type_;

	vertex_count = vertex_count_;
	index_count = index_count_;

	gfx::BufferAllocInfo vertex_info = {};
	vertex_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	vertex_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vertex_info.size = GetVertexBufferSize();

	gfx::BufferAllocInfo index_info = {};
	index_info.usage = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	index_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	index_info.size = GetIndexBufferSize();

	gfx::Device *device = gfx::Device::Get();

	vertex_buffer = device->BufferAlloc(vertex_info);
	index_buffer = device->BufferAlloc(index_info);
}

void rnd::Mesh::Destroy() const
{
	gfx::Device *device = gfx::Device::Get();

	device->BufferDestroy(vertex_buffer);
	device->BufferDestroy(index_buffer);
}

void rnd::Mesh::WriteToStage(
	gfx::ResourceKey stage,
	u64 stage_base,
	const void *vertices,
	const void *indices
) const
{
	u64 vb_size = GetVertexBufferSize();
	u64 ib_size = GetIndexBufferSize();

	gfx::Device *device = gfx::Device::Get();

	device->BufferWrite(stage, vertices, vb_size, stage_base);
	device->BufferWrite(stage, indices, ib_size, stage_base + vb_size);
}

u64 rnd::Mesh::RecordUpload(const gfx::CmdBuffer &cmd, gfx::ResourceKey stage, u64 stage_base) const
{
	u64 vb_size = GetVertexBufferSize();
	u64 ib_size = GetIndexBufferSize();

	gfx::BufferCopy stage_to_vertex_copy = {};
	stage_to_vertex_copy.src_offset = stage_base;
	stage_to_vertex_copy.dst_offset = 0;
	stage_to_vertex_copy.size = vb_size;

	gfx::BufferCopy stage_to_index_copy = {};
	stage_to_index_copy.src_offset = stage_base + vb_size;
	stage_to_index_copy.dst_offset = 0;
	stage_to_index_copy.size = ib_size;

	cmd.CopyBufferToBuffer(stage, vertex_buffer, 1, &stage_to_vertex_copy);
	cmd.CopyBufferToBuffer(stage, index_buffer, 1, &stage_to_index_copy);

	return vb_size + ib_size;
}

// TODO: TODO: TODO: ARE THESE REALLY NECESSARY? CAN JUST REMOVE THEM AND USE CmdBuffer CALLS DIRECTLY NO?
void rnd::Mesh::UploadBlocking(const void *vertices, const void *indices) const
{
	gfx::Device *device = gfx::Device::Get();

	gfx::ResourceKey stage = device->StageAlloc(GetVertexBufferSize() + GetIndexBufferSize());

	WriteToStage(stage, 0, vertices, indices);

	gfx::CmdBuffer cmd = device->SubmitImBegin();
	RecordUpload(cmd, stage, 0);
	device->SubmitImEnd(cmd);

	device->BufferDestroy(stage);
}

void rnd::Mesh::RecordBindIndexBuffer(const gfx::CmdBuffer &cmd) const
{
	cmd.BindIndexBuffer(index_buffer, 0, VK_WHOLE_SIZE, index_type);
}

void rnd::Mesh::Draw(const gfx::CmdBuffer &cmd) const
{
	cmd.DrawIndexed(index_count, 1, 0, 0, 0);
}

void rnd::Mesh::DrawInstanced(const gfx::CmdBuffer &cmd, unsigned first) const
{
	cmd.DrawIndexed(index_count, 1, 0, 0, first);
}
