#ifndef RENDER_MESH_H
#define RENDER_MESH_H

#include "core/types.h"
#include "graphics/mesh.h"
#include "graphics/resource_key.h"
#include "graphics/cmd_buffer.h"

namespace rnd {

struct Mesh {
	u64 vertex_stride = 0;
	gfx::IndexType index_type = gfx::IndexType_U32;

	unsigned vertex_count = 0;
	unsigned index_count = 0;

	gfx::ResourceKey vertex_buffer;
	gfx::ResourceKey index_buffer;

	void Alloc(
		u64 vertex_stride,
		gfx::IndexType index_type,
		unsigned vertex_count,
		unsigned index_count
	);

	void Destroy() const;

	void WriteToStage(
		gfx::ResourceKey stage,
		u64 stage_base,
		const void *vertices,
		const void *indices
	) const;

	u64 RecordUpload(const gfx::CmdBuffer &cmd, gfx::ResourceKey stage, u64 stage_base) const;

	void UploadBlocking(const void *vertices, const void *indices) const;

	void RecordBindIndexBuffer(const gfx::CmdBuffer &cmd) const;

	void Draw(const gfx::CmdBuffer &cmd) const;
	void DrawInstanced(const gfx::CmdBuffer &cmd, unsigned first) const;

	u64 GetVertexBufferSize() const
	{
		return vertex_count * vertex_stride;
	}

	u64 GetIndexBufferSize() const
	{
		return index_count * gfx::IndexTypeStride(index_type);
	}
};

}

#endif // RENDER_MESH_H
