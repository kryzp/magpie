#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

#include "core/types.h"
#include "core/maths.h"
#include "core/log.h"
#include "core/pool.h"
#include "graphics/mesh.h"
#include "graphics/resource_key.h"
#include "asset/types.h"
#include "render/gpu_types.h"
#include "render/geometry.h"
#include "render/light.h"
#include "render/material.h"

#define RENDER_SCENE_MAX_ENTITIES             4096
#define RENDER_SCENE_MAX_MESHES               4096
#define RENDER_SCENE_MAX_RETIRED_REGIONS      4096
#define RENDER_SCENE_MAX_GEOMETRY_PAGES       64
#define RENDER_SCENE_MAX_MATERIALS            4096

namespace rnd {

struct MaterialHandle {
	unsigned index = (unsigned)-1;

	static MaterialHandle Null()
	{
		return {};
	}
};

struct MeshHandle {
	unsigned slot_index = (unsigned)-1;
	unsigned page_index = (unsigned)-1;

	static MeshHandle Null()
	{
		return {};
	}
};

struct MeshDesc {
	const void *vertices = nullptr;
	unsigned vertex_count = 0;
	u64 vertex_stride = 0;

	const void *indices = nullptr;
	unsigned index_count = 0;
	gfx::IndexType index_type = gfx::IndexType_U32;

	gfx::ResourceKey skin_buffer;
};

struct MeshAllocRegion {
	unsigned page_index = 0;
	u64 vertex_offset = 0;
	unsigned vertex_count = 0;
	u64 index_offset = 0;
	unsigned index_count = 0;
};

struct SceneRetiredRegion {
	MeshAllocRegion region;
	u64 retire_frame = 0;
};

struct Object {
	Mat4 transform;
	Mat4 normal_matrix;
	Mat4 prev_transform;
	bool has_transform = false;

	Vec4 local_sphere_bounds;

	unsigned page_index = 0;
	unsigned mesh_index = 0;
	unsigned material_index = 0;

	const Mat4 *skinning_palette = nullptr;
	unsigned skinning_joint_count = 0;
};

enum EntityType {
	EntityType_Object,
	EntityType_Light,
	EntityType_MaxEnum
};

struct EntityHandle {
	unsigned id = 0;
	EntityType type = EntityType_Object;
};

// todo: should use union, but doesnt because of ctors
struct Entity {
	EntityType type = EntityType_Object;
	Object object;
	Light light;
};

class Scene {
public:
	explicit Scene(LogChannel log_channel);
	~Scene();

	Scene(const Scene &) = delete;
	Scene &operator=(const Scene &) = delete;


	/* ==================================================
	   CORE SCENE
	   ================================================== */

	void BeginFrame(u64 frame_number, bool has_completed, u64 gpu_completed_frame);

	void BeginSimSteps(unsigned step_count);
	[[nodiscard]] float InterpolationFactor(float alpha) const;
	[[nodiscard]] Mat4 ObjectRenderTransform(EntityHandle handle, float interp_t) const;


	/* ==================================================
	   ENTITIES
	   ================================================== */

	EntityHandle EntityCreate(EntityType type);
	void EntityDestroy(EntityHandle handle);

	[[nodiscard]] unsigned EntityLiveCount() const
	{
		return entity_pool.LiveCount();
	}

	[[nodiscard]] const Entity &EntityAt(unsigned dense_index) const
	{
		return entities[dense_index];
	}

	void SetObjectTransform(EntityHandle handle, const Mat4 &transform);
	void TeleportObject(EntityHandle handle, const Mat4 &transform); // no interpolation
	void SetObjectLocalSphereBounds(EntityHandle handle, const Vec4 &local_sphere_bounds);
	void SetObjectMesh(EntityHandle handle, MeshHandle mesh);
	void SetObjectMaterial(EntityHandle handle, MaterialHandle material);
	void SetObjectSkinning(EntityHandle handle, const Mat4 *palette, unsigned joint_count);

	void SetLightParam(EntityHandle handle, const Light &light);


	/* ==================================================
	   MESHES
	   ================================================== */

	MeshHandle AllocMesh(const MeshDesc &desc);
	void FreeMesh(MeshHandle handle);

	[[nodiscard]] unsigned CountOfMeshes() const;

	[[nodiscard]] unsigned MeshHighWater() const
	{
		return mesh_high_water;
	}

	[[nodiscard]] const GPU_RenderMesh *GPUMeshes() const
	{
		return gpu_meshes;
	}

	unsigned FindSuitablePage(
		unsigned vertex_count, u64 vertex_stride,
		unsigned index_count, gfx::IndexType index_type
	);

	GeometryPage CreateNewPage(u64 vertex_stride, gfx::IndexType index_type);

	[[nodiscard]] unsigned PageCount() const
	{
		return geometry_page_count;
	}

	[[nodiscard]] const GeometryPage &PageAt(unsigned index) const
	{
		return geometry_pages[index];
	}


	/* ==================================================
	   MATERIALS
	   ================================================== */

	MaterialHandle AddMaterial(const Material &material);
	MaterialHandle AddMaterialFromAssets(const ast::ModelMaterial &source);

	void UpdateMaterial(MaterialHandle handle, const Material &material);
	void FreeMaterial(MaterialHandle handle);

	[[nodiscard]] unsigned CountOfMaterials() const;

	[[nodiscard]] unsigned MaterialHighWater() const
	{
		return material_high_water;
	}

	[[nodiscard]] const GPU_Material *GPUMaterials() const
	{
		return gpu_materials;
	}

	[[nodiscard]] const Material *GetMaterial(MaterialHandle handle) const;

	void BakeMaterialIntoGPU(const Material &material, GPU_Material *out) const;

private:
	void ReleaseRegion(const MeshAllocRegion &region);

	LogChannel log_channel;

	DensePool entity_pool;
	Entity entities[RENDER_SCENE_MAX_ENTITIES];
	unsigned entity_count[EntityType_MaxEnum];

	u64 frame_number;

	unsigned last_sim_step_count;
	u64 gpu_completed_frame;
	bool any_gpu_completed;

	SlotPool mesh_pool;
	unsigned mesh_high_water; // highest slot we've handed out so far + 1
	MeshAllocRegion mesh_allocs[RENDER_SCENE_MAX_MESHES];
	GPU_RenderMesh gpu_meshes[RENDER_SCENE_MAX_MESHES];

	unsigned retired_region_count;
	SceneRetiredRegion retired_regions[RENDER_SCENE_MAX_RETIRED_REGIONS];

	GeometryPage geometry_pages[RENDER_SCENE_MAX_GEOMETRY_PAGES];
	unsigned geometry_page_count;

	SlotPool material_pool;
	unsigned material_high_water;
	Material cpu_materials[RENDER_SCENE_MAX_MATERIALS];
	GPU_Material gpu_materials[RENDER_SCENE_MAX_MATERIALS];
};

}

#endif // RENDER_SCENE_H
