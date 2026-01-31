#pragma once

#include "core/types.h"
#include "container/vector.h"
#include "math/matrix.h"
#include "math/vec3.h"
#include "math/colour.h"

#include "gpu_types.h"
#include "model.h"

namespace gfx
{
	class Device;
	class GpuBuffer;
	class CommandBuffer;

	struct Light {
		enum LightType {
			TYPE_POINT,
			TYPE_MAX_ENUM
		};

		LightType type;
		Vec3 position;
		Vec3 direction;
		Colour colour;
		float intensity;
		float falloff;
	};

	struct RenderHandle {
		u32 index;
		u32 generation;
	};

	/*
	 * Memory is allocated in pages.
	 */
	struct GeometryPage {
		const GpuBuffer *vertex_buffer;
		const GpuBuffer *index_buffer;

		const GpuBuffer *indirect_buffer;
		const GpuBuffer *draw_count_buffer;

		u32 vertex_offset = 0;
		u32 index_offset = 0;
		u32 max_vertices;
		u32 max_indices;
	};

	class RenderScene {
	public:
		static constexpr u32 PAGE_MAX_OBJECTS = 512;
		static constexpr u32 INITIAL_MAX_OBJECTS = 512;
		static constexpr u32 INITIAL_MAX_LIGHTS = 1024;
		static constexpr u32 INITIAL_MAX_MESHES = 1024;
		static constexpr u32 INITIAL_MAX_MATERIALS = 128;
		static constexpr u32 INITIAL_MAX_PAGES = 64;

		RenderScene();
		~RenderScene();

		void init(Device *device);
		void destroy();

		void update_gpu_buffers();

		bool is_valid_object(RenderHandle handle) const;
		bool is_valid_light(RenderHandle handle) const;

		RenderHandle create_object(
			const Mat4 &transform,
			u32 mesh, u32 material
		);

		void remove_object(RenderHandle handle);

		void set_transform(RenderHandle handle, const Mat4 &transform);

		RenderHandle create_light(const Light &light);
		void remove_light(RenderHandle handle);

		void set_light_colour(RenderHandle handle, const Vec3 &colour);
		void set_light_intensity(RenderHandle handle, float intensity);

		Mat4 get_light_view(RenderHandle handle) const;
		Mat4 get_light_proj(RenderHandle handle) const;

		u32 register_mesh(const Mesh &mesh);
		u32 register_material(const Material &material, ast::AssetManager &assets);

		u32 get_object_count() const;
		u32 get_light_count() const;

		const GpuBuffer *get_object_buffer() const;
		const GpuBuffer *get_light_buffer() const;
		const GpuBuffer *get_material_buffer() const;
		const GpuBuffer *get_mesh_buffer() const;
		const GpuBuffer *get_page_buffer() const;

		const Vector<GeometryPage> &get_geometry_pages() const;

	private:
		Device *device;

		/*
		 * This is a little confusing so for later reference:
		 *
		 * --> We need tight memory packing for the GPU upload but we need
		 *     stable pointers at the same time.
		 *
		 *     So basically, we store an extra level of indirection, which
		 *     we call a handle entry.
		 *
		 *     It maps from the "user" handle (RenderHandle)
		 *     to the actual internal index of the resource.
		 *
		 *         RenderHandle (index, generation)
		 *      => HandleEntry[index] (dense_index, generation)
		 *      => ObjectData[dense_index] (gpu data)
		 *
		 *     The generation parameter is used to make sure the handle
		 *     hasn't gone stale.
		 */
		struct HandleEntry {
			u32 dense_index;
			u32 generation;
		};

		struct {
			Vector<HandleEntry> handles;
			Vector<u32> free_indices;

			Vector<RenderHandle> back_references;

			Vector<Mat4> transforms;
			Vector<Vec4> sphere_bounds;
			Vector<u32> meshes;
			Vector<u32> materials;
			Vector<u32> page_indices;
		} objects;
		
		struct {
			Vector<HandleEntry> handles;
			Vector<u32> free_indices;

			Vector<RenderHandle> back_references;

			Vector<gpu_types::GpuLight> data;
		} lights;

		Vector<GeometryPage> geometry_pages;
		GpuBuffer *page_table_buffer;
		
		struct MeshMemoryLocation {
			u32 page;
			u32 index;
		};

		Vector<MeshMemoryLocation> mesh_registry;

		Vector<gpu_types::GpuMesh> meshes;
		GpuBuffer *mesh_buffer;

		Vector<gpu_types::GpuMaterial> materials;
		GpuBuffer *material_buffers[FRAMES_IN_FLIGHT];

		GpuBuffer *object_buffers[FRAMES_IN_FLIGHT];
		GpuBuffer *light_buffers[FRAMES_IN_FLIGHT];

		u32 alloc_handle_index(Vector<HandleEntry> &map, Vector<u32> &free_list);

		u32 find_suitable_page(u32 vertex_count, u32 index_count);
		GeometryPage create_new_page();
	};
}
