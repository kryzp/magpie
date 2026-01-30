#pragma once

#include "container/vector.h"

#include "math/vec2.h"
#include "math/colour.h"

#include "texture.h"
#include "gpu_buffer.h"
#include "model.h"
#include "camera.h"
#include "device.h"
#include "gpu_types.h"
#include "render_graph.h"

namespace gfx
{
	struct Light {
		enum LightType {
			TYPE_POINT,
			TYPE_MAX_ENUM
		};

		LightType type;
		Vec3 direction;
		Colour colour;
		float intensity;
		float falloff;
	};

	class RenderScene;

	struct RenderMesh {
		const Mesh *original;
		bool is_merged;
		u32 first_vertex;
		u32 first_index;
		u32 vertex_count;
		u32 index_count;
	};

	constexpr static u32 RENDER_SCENE_INVALID_HANDLE = -1u;

	static inline bool render_scene_handle_valid(u32 h)
	{
		return h != RENDER_SCENE_INVALID_HANDLE && h >= 0;
	};

	struct RenderSceneObject {
		u32 id = 0;
		Mat4 transform = Mat4::identity();
		//Bounds3D bounds;
		u32 flags = 0;
		u64 custom_sort_key = 0;
		u32 mesh_handle = RENDER_SCENE_INVALID_HANDLE;
		u32 material_handle = RENDER_SCENE_INVALID_HANDLE;
		u32 light_handle = RENDER_SCENE_INVALID_HANDLE;
	};

	struct MeshPass {
		enum Type {
			TYPE_FORWARD,
			TYPE_MAX_ENUM
		};

		struct MultiBatch {
			u32 first;
			u32 count;
		};

		struct IndirectBatch {
			u32 mesh_id;
			u32 material_id;
			u32 first;
			u32 count;
		};

		struct DirectBatch {
			u32 object_id;
			//u64 sort_key;
		};

		Vector<MultiBatch> multi_batches;
		Vector<IndirectBatch> batches;
		Vector<DirectBatch> direct_batches;

		RenderResourceHandle instance_buffer;           // <gpu_types::Instance>
		RenderResourceHandle compacted_instance_buffer; // <u32>
		RenderResourceHandle draw_indirect_buffer;      // <gpu_types::Indirect>
//		RenderResourceHandle clear_indirect_buffer;     // <gpu_types::Indirect>

		bool instance_buffer_needs_refresh;
		bool indirect_buffer_needs_refresh;

		void push_stage(RenderGraph &graph);

		void populate(RenderScene &scene);

		void fill_instance_array(const RenderScene &scene, gpu_types::GpuInstance *instances);
		void fill_indirect_array(const RenderScene &scene, gpu_types::GpuIndirect *indirects);
	};

	class RenderScene {
	public:
		constexpr static u32 MAX_OBJECTS = 4096;
		constexpr static u32 MAX_MATERIALS = 64;

		RenderScene() = default;
		~RenderScene() = default;

		void init(Device *device);
		void destroy();

		void update();

		void mesh_pass_stages(RenderGraph &graph);

		void remove_object(u32 handle);
		void resolve_removing();

		u32 create_object(const Mat4 &transform);

		u32 upload_mesh(const Mesh &mesh);
		u32 upload_material(const Material &material, ast::AssetManager &assets);
		u32 upload_light(const Light &light);

		RenderMesh *get_mesh(u32 handle);
		const RenderMesh *get_mesh(u32 handle) const;
		
		Material *get_material(u32 handle);
		const Material *get_material(u32 handle) const;
		
		Light *get_light(u32 handle);
		const Light *get_light(u32 handle) const;

		void build_batches();
		void merge_meshes();

		RenderSceneObject &get_object_from_handle(u32 handle);
		
		MeshPass &get_pass(MeshPass::Type type);
		const MeshPass &get_pass(MeshPass::Type type) const;

		const Vector<RenderSceneObject> &get_objects() const;

		u32 get_light_count() const;

		const GpuBuffer *get_object_buffer() const;
		const GpuBuffer *get_material_buffer() const;
		const GpuBuffer *get_light_buffer() const;

		const GpuBuffer *get_merged_vertex_buffer() const;
		const GpuBuffer *get_merged_index_buffer() const;

	private:
		Device *device;

		Vector<RenderSceneObject> objects;

		Vector<u32> pending_removals;
		Vector<u32> reusable_handles;

		MeshPass passes[MeshPass::TYPE_MAX_ENUM];

		Vector<RenderMesh> meshes;
		Vector<Material> materials;
		Vector<Light> lights;

		GpuBuffer *object_buffer;
		GpuBuffer *material_buffer;
		GpuBuffer *light_buffer;

		GpuBuffer *merged_vertex_buffer;
		GpuBuffer *merged_index_buffer;
	};
}
