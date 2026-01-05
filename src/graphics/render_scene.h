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

struct EnvironmentProbe {
	Texture irradiance;
	Texture prefilter;
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

constexpr static u32 RS_HANDLE_INVALID_INDEX = (u32)(-1);

static inline bool render_scene_handle_valid(u32 h)
{
	return h != RS_HANDLE_INVALID_INDEX && h >= 0;
};

struct RenderSceneObject {
	u32 id = 0;
	Mat4 transform = Mat4::identity();
	//Bounds3D bounds;
	u32 flags = 0;
	u64 custom_sort_key = 0;
	u32 mesh_handle = RS_HANDLE_INVALID_INDEX;
	u32 material_handle = RS_HANDLE_INVALID_INDEX;
	u32 light_handle = RS_HANDLE_INVALID_INDEX;
};

struct SceneView {
	const RenderScene *scene;
	const Camera *camera;
};

class RenderScene {
	struct MeshPass {
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

		enum MeshPassType {
			TYPE_FORWARD,
			TYPE_MAX_ENUM
		};

		Vector<MultiBatch> multi_batches;
		Vector<IndirectBatch> batches;
		Vector<DirectBatch> direct_batches;

		GpuBuffer compacted_instance_buffer; // <u32>
		GpuBuffer instance_buffer;           // <gpu_types::Instance>
		GpuBuffer draw_indirect_buffer;      // <gpu_types::Indirect>
		GpuBuffer clear_indirect_buffer;     // <gpu_types::Indirect>

		bool instance_buffer_needs_refresh;
		bool indirect_buffer_needs_refresh;

		void init(Device *device);
		void destroy(Device *device);

		void populate(RenderScene &scene);

		void fill_instance_array(const RenderScene &scene, gpu_types::GpuInstance *instances);
		void fill_indirect_array(const RenderScene &scene, gpu_types::GpuIndirect *indirects);
	};

public:
	constexpr static u32 MAX_OBJECTS = 128;
	constexpr static u32 MAX_MATERIALS = 64;

	RenderScene() = default;
	~RenderScene() = default;

	void init(Device *device);
	void destroy();

	void update();

	void remove_object(u32 handle);
	void resolve_removing();

	u32 create_object(const Mat4 &transform);

	u32 upload_mesh(const Mesh &mesh);
	u32 upload_material(const Material &material);
	u32 upload_light(const Light &light);

	void build_batches();
	void merge_meshes();

	RenderSceneObject &get_object_from_handle(u32 handle);

private:
	Device *device;

	Vector<RenderSceneObject> objects;

	Vector<u32> pending_removals;
	Vector<u32> reusable_handles;

	MeshPass passes[MeshPass::TYPE_MAX_ENUM];

	Vector<RenderMesh> meshes;
	Vector<Material> materials;
	Vector<Light> lights;

	GpuBuffer object_buffer;
	GpuBuffer material_buffer;
	GpuBuffer light_buffer;

	GpuBuffer merged_vertex_buffer;
	GpuBuffer merged_index_buffer;
};

}
