#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

#define R_SCENE_MAX_ENTITIES           1024
#define R_SCENE_MAX_MESHES             1024
#define R_SCENE_MAX_MESH_COPIES        128
#define R_SCENE_MAX_GEOMETRY_PAGES     16
#define R_SCENE_MAX_MATERIALS          1024

typedef struct R_MaterialHandle R_MaterialHandle;
struct R_MaterialHandle
{
	u32 index;
};

typedef struct R_MeshHandle R_MeshHandle;
struct R_MeshHandle
{
	u32 slot_index;
	u32 page_index;
};

internal inline R_MaterialHandle R_MaterialHandleNull(void)
{
	R_MaterialHandle null_handle = {0};
	null_handle.index = (u32)(-1);
	
	return null_handle;
}

internal inline R_MeshHandle R_MeshHandleNull(void)
{
	R_MeshHandle null_handle = {0};
	null_handle.slot_index = (u32)(-1);
	null_handle.page_index = (u32)(-1);
	
	return null_handle;
}

typedef struct R_MeshDesc R_MeshDesc;
struct R_MeshDesc
{
	const void *vertices;
	u32 vertex_count;
	
	const void *indices;
	u32 index_count;

	G_ResourceKey skin_buffer;
};

typedef struct R_MeshAllocRegion R_MeshAllocRegion;
struct R_MeshAllocRegion
{
	u32 page_index;

	u64 vertex_offset;
	u32 vertex_count;

	u64 index_offset;
	u32 index_count;
};

typedef struct R_ScenePageMeshCopy R_ScenePageMeshCopy;
struct R_ScenePageMeshCopy
{
	const void *vertices;
	u64 vertex_size;
	uptr vertex_offset_dst;

	const void *indices;
	u64 index_size;
	uptr index_offset_dst;

	u32 dst_page_index;
};

typedef struct R_Object R_Object;
struct R_Object
{
	m4 transform;
	m4 normal_matrix;

	v4 local_sphere_bounds;

	u32 page_index;
	u32 mesh_index;
	u32 material_index;

	const m4 *skinning_palette;
	u64 skinning_address;
	u32 skinning_joint_count;
};

typedef enum R_EntityType
{
	R_EntityType_Object,
	R_EntityType_Light,
	R_EntityType_COUNT
}
R_EntityType;

typedef struct R_EntityHandle R_EntityHandle;
struct R_EntityHandle
{
	u32 id;
	R_EntityType type;
};

typedef struct R_Entity R_Entity;
struct R_Entity
{
	R_EntityType type;
	
	union
	{
		R_Object object;
		R_Light light;
	};
};

typedef struct R_Scene R_Scene;

typedef struct R_EntityIterator R_EntityIterator;
struct R_EntityIterator
{
	R_Scene *scene;
	u32 index;
};

internal R_EntityIterator R_EntityIteratorInit(R_Scene *scene);
internal void R_EntityIteratorReset(R_EntityIterator *iter);
internal R_Entity *R_EntityIteratorNext(R_EntityIterator *iter, R_EntityType type);

typedef struct R_Scene R_Scene;
struct R_Scene
{
	LOG_Channel log_channel;

	DensePool entity_pool;
	R_Entity entities[R_SCENE_MAX_ENTITIES];
	b32 entity_occupied[R_SCENE_MAX_ENTITIES];
	u32 entity_count[R_EntityType_COUNT];
	
	SlotPool mesh_pool;
	R_MeshAllocRegion mesh_allocs[R_SCENE_MAX_MESHES];
	R_GPU_RenderMesh gpu_meshes[R_SCENE_MAX_MESHES];
	G_ResourceKey mesh_buffer;
	b32 mesh_buffer_dirty;

	R_ScenePageMeshCopy page_mesh_copies[R_SCENE_MAX_MESH_COPIES];
	u32 page_mesh_copy_count;
	
	R_GeometryPage geometry_pages[R_SCENE_MAX_GEOMETRY_PAGES];
	u32 geometry_page_count;

	SlotPool material_pool;
	R_Material cpu_materials[R_SCENE_MAX_MATERIALS];
	R_GPU_Material gpu_materials[R_SCENE_MAX_MATERIALS];
	G_ResourceKey material_buffer;
	b32 material_buffer_dirty;
};


/* ==================================================
   CORE SCENE
   ================================================== */

internal void                  R_SceneInit(R_Scene *scene, Arena *arena, LOG_Channel log_channel);
internal void                  R_SceneDestroy(R_Scene *scene);


/* ==================================================
   ENTITIES
   ================================================== */

internal R_EntityHandle        R_SceneEntityCreate(R_Scene *scene, R_EntityType type);
internal void                  R_SceneEntityDestroy(R_Scene *scene, R_EntityHandle handle);

internal void                  R_SceneSetObjectTransform(R_Scene *scene, R_EntityHandle handle, m4 transform);
internal void                  R_SceneSetObjectLocalSphereBounds(R_Scene *scene, R_EntityHandle handle, v4 local_sphere_bounds);
internal void                  R_SceneSetObjecteMesh(R_Scene *scene, R_EntityHandle handle, R_MeshHandle mesh);
internal void                  R_SceneSetObjectMaterial(R_Scene *scene, R_EntityHandle handle, R_MaterialHandle material);
internal void                  R_SceneSetObjectSkinning(R_Scene *scene, R_EntityHandle handle, const m4 *palette, u32 joint_count);

internal void                  R_SceneSetLightParam(R_Scene *scene, R_EntityHandle handle, R_Light light);


/* ==================================================
   MESHES
   ================================================== */

internal R_MeshHandle          R_SceneAllocMesh(R_Scene *scene, const R_MeshDesc *desc);
internal void                  R_SceneFreeMesh(R_Scene *scene, R_MeshHandle handle);

internal u32                   R_SceneCountOfMeshes(const R_Scene *scene);

internal u32                   R_SceneFindSuitablePage(R_Scene *scene, u32 vertex_count, u32 index_count);
internal R_GeometryPage        R_SceneCreateNewPage(R_Scene *scene);
internal u32                   R_ScenePageCount(const R_Scene *scene);


/* ==================================================
   MATERIALS
   ================================================== */

internal R_MaterialHandle      R_SceneAddMaterial(R_Scene *scene, const R_Material *material);
internal R_MaterialHandle      R_SceneAddMaterialFromAssets(R_Scene *scene, const A_ModelMaterial *source);

internal void                  R_SceneUpdateMaterial(R_Scene *scene, R_MaterialHandle handle, const R_Material *material);
internal void                  R_SceneFreeMaterial(R_Scene *scene, R_MaterialHandle handle);

internal u32                   R_SceneCountOfMaterials(const R_Scene *scene);

internal const R_Material     *R_SceneGetMaterial(const R_Scene *scene, R_MaterialHandle handle);

internal void                  R_SceneFlushIfDirty(R_Scene *scene);
internal void                  R_SceneBakeMaterialIntoGPU(const R_Scene *scene, const R_Material *material, R_GPU_Material *out);
internal u32                   R_SceneResolveToBindlessIndex(const R_Scene *scene, G_ResourceKey key);


#endif // RENDER_SCENE_H
