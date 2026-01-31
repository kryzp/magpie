#pragma once

#include "core/types.h"
#include "assets/assets.h"
#include "container/vector.h"

#include "command_buffer.h"
#include "gpu_buffer.h"

namespace gfx
{
	class Mesh {
	public:
		Mesh() = default;
		~Mesh() = default;

		void init(
			Device *device, u64 vertex_size,
			u32 vertex_count, void *vertices,
			u32 index_count, u32 *indices
		);

		void destroy() const;

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
		ast::AssetHandle diffuse;
		ast::AssetHandle normal;
		ast::AssetHandle emissive;
		ast::AssetHandle metallic_roughness;
		ast::AssetHandle ambient;
	};

	struct SubModel {
		Mesh mesh;
		Material material;
	};

	class Model {
	public:
		Model()
			: sub_models()
		{
		}

		~Model() = default;

		SubModel &add_sub_model()
		{
			return sub_models.emplace_back();
		}

		const SubModel &get_sub_model(int index) const
		{
			return sub_models[index];
		}

	private:
		Vector<SubModel> sub_models;
	};
}
