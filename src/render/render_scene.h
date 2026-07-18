#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

#define R_SCENE_MAX_INSTANCES          65536
#define R_SCENE_MAX_LIGHTS             1024
#define R_SCENE_MAX_MESHES             1024
#define R_SCENE_MAX_GEOMETRY_PAGES     16
#define R_SCENE_MAX_MATERIALS          1024

typedef struct R_InstanceHandle R_InstanceHandle;
struct R_InstanceHandle
{
	u32 id;
};

typedef struct R_LightHandle R_LightHandle;
struct R_LightHandle
{
	u32 id;
};

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

static inline R_MaterialHandle R_MaterialHandleNull(void)
{
	R_MaterialHandle null_handle = {0};
	null_handle.index = (u32)(-1);
	
	return null_handle;
}

static inline R_MeshHandle R_MeshHandleNull(void)
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

	G_BufferKey skin_buffer;
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

	v4 sphere_bounds;

	u32 page_index;
	u32 mesh_index;
	u32 material_index;

	const m4 *skinning_palette;
	u64 skinning_address;
	u32 skinning_joint_count;
};

typedef struct R_Scene R_Scene;
struct R_Scene
{
	LOG_Channel log_channel;

	DensePool object_pool;
	R_Object objects[R_SCENE_MAX_INSTANCES];
	b32 object_occupied[R_SCENE_MAX_INSTANCES];
	
	DensePool light_pool;
	R_Light lights[R_SCENE_MAX_LIGHTS];
	b32 light_occupied[R_SCENE_MAX_LIGHTS];

	SlotPool mesh_pool;
	R_MeshAllocRegion mesh_allocs[R_SCENE_MAX_MESHES];
	R_GPU_RenderMesh gpu_meshes[R_SCENE_MAX_MESHES];
	G_BufferKey mesh_buffer;
	b32 mesh_buffer_dirty;

	R_ScenePageMeshCopy page_mesh_copies[128];
	u32 page_mesh_copy_count;
	
	R_GeometryPage geometry_pages[R_SCENE_MAX_GEOMETRY_PAGES];
	u32 geometry_page_count;

	SlotPool material_pool;
	R_Material cpu_materials[R_SCENE_MAX_MATERIALS];
	R_GPU_Material gpu_materials[R_SCENE_MAX_MATERIALS];
	G_BufferKey material_buffer;
	b32 material_buffer_dirty;
};


/* ==================================================
   CORE SCENE
   ================================================== */

static void                  R_SceneInit(R_Scene *scene, Arena *arena, LOG_Channel log_channel);
static void                  R_SceneDestroy(R_Scene *scene);


/* ==================================================
   INSTANCES
   ================================================== */

static R_InstanceHandle      R_SceneInstanceCreate(R_Scene *scene);
static void                  R_SceneInstanceDestroy(R_Scene *scene, R_InstanceHandle handle);
static void                  R_SceneSetInstanceTransform(R_Scene *scene, R_InstanceHandle handle, m4 transform);
static void                  R_SceneSetInstanceSphereBounds(R_Scene *scene, R_InstanceHandle handle, v4 sphere_bounds);
static void                  R_SceneSetInstanceMesh(R_Scene *scene, R_InstanceHandle handle, R_MeshHandle mesh);
static void                  R_SceneSetInstanceMaterial(R_Scene *scene, R_InstanceHandle handle, R_MaterialHandle material);
static void                  R_SceneSetInstanceSkinning(R_Scene *scene, R_InstanceHandle handle, const m4 *palette, u32 joint_count);


/* ==================================================
   LIGHTS
   ================================================== */

static R_LightHandle         R_SceneLightCreate(R_Scene *scene);
static void                  R_SceneLightDestroy(R_Scene *scene, R_LightHandle handle);
static void                  R_SceneSetLight(R_Scene *scene, R_LightHandle handle, R_Light light);


/* ==================================================
   MESHES
   ================================================== */

static R_MeshHandle          R_SceneAllocMesh(R_Scene *scene, const R_MeshDesc *desc);
static void                  R_SceneFreeMesh(R_Scene *scene, R_MeshHandle handle);

static u32                   R_SceneCountOfMeshes(const R_Scene *scene);

static u32                   R_SceneFindSuitablePage(R_Scene *scene, u32 vertex_count, u32 index_count);
static R_GeometryPage        R_SceneCreateNewPage(R_Scene *scene);
static u32                   R_ScenePageCount(const R_Scene *scene);


/* ==================================================
   MATERIALS
   ================================================== */

static R_MaterialHandle      R_SceneAddMaterial(R_Scene *scene, const R_Material *material);
static R_MaterialHandle      R_SceneAddMaterialFromAssets(R_Scene *scene, const A_ModelMaterial *source);

static void                  R_SceneUpdateMaterial(R_Scene *scene, R_MaterialHandle handle, const R_Material *material);
static void                  R_SceneFreeMaterial(R_Scene *scene, R_MaterialHandle handle);

static u32                   R_SceneCountOfMaterials(const R_Scene *scene);

static const R_Material     *R_SceneGetMaterial(const R_Scene *scene, R_MaterialHandle handle);

static void                  R_SceneFlushIfDirty(R_Scene *scene);
static void                  R_SceneBakeMaterialIntoGPU(const R_Scene *scene, const R_Material *material, R_GPU_Material *out);
static u32                   R_SceneResolveToBindlessIndex(const R_Scene *scene, G_TextureKey key);


#endif // RENDER_SCENE_H
