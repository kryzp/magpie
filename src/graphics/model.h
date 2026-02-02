#pragma once

#include "core/types.h"
#include "assets/assets.h"
#include "container/vector.h"
#include "math/mat4.h"

#include "command_buffer.h"
#include "gpu_buffer.h"

namespace gfx
{
	typedef u32 IndexType;

	class Mesh {
	public:
		Mesh();
		~Mesh();

		void create_buffers(
			Device *device, u64 vertex_size,
			u32 vertex_count, u32 index_count
		);

		void destroy_buffers() const;

		void write_to_staging_buffer(
			GpuBuffer *staging_buffer, u64 offset,
			void *vertices, IndexType *indices
		);

		// Returns the offset for this mesh.
		u64 batch_upload(
			CommandBuffer &cmd,
			GpuBuffer *staging_buffer, u64 offset
		);

		u64 get_vertex_buffer_size() const
		{
			return vertex_count * vertex_size;
		}

		u64 get_index_buffer_size() const
		{
			return index_count * sizeof(IndexType);
		}

		void bind_indices(CommandBuffer &cmd) const
		{
			cmd.bind_index_buffer(index_buffer, 0);
		}

		void draw_indexed(CommandBuffer &cmd, u32 instance_id = 0) const
		{
			cmd.draw_indexed(index_count, 1, 0, 0, instance_id);
		}

		u64 vertex_size;

		u32 vertex_count;
		u32 index_count;

		GpuBuffer *vertex_buffer;
		GpuBuffer *index_buffer;

	private:
		Device *device;
	};

	struct Material {
		ast::AssetHandle albedo;
		ast::AssetHandle normal;
		ast::AssetHandle emissive;
		ast::AssetHandle metallic_roughness;
		ast::AssetHandle ambient;
	};

	struct SubModel {
		Mat4 transform;
		Mesh mesh;
		Material material;
	};

	struct Model {
		Vector<SubModel> sub_models;
	};
}
