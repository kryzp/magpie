#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

#define R_SCENE_MAX_OBJECTS                 1024
#define R_SCENE_MAX_LIGHTS                   128
#define R_SCENE_MAX_MATERIALS               1024
#define R_SCENE_MAX_MESHES                  1024
#define R_SCENE_MAX_SHADOW_CASTERS             8
#define R_SCENE_MAX_GEOMETRY_PAGES            16

typedef struct R_SceneHandle R_SceneHandle;
struct R_SceneHandle
{
	u32 index;
	u32 generation;
};

internal b32 R_SceneHandleIsNull(R_SceneHandle h);

typedef struct R_MeshDesc R_MeshDesc;
struct R_MeshDesc
{
	G_BufferKey vertex_buffer;
	G_BufferKey index_buffer;

	u32 vertex_count;
	u32 index_count;

	G_BufferKey skin_buffer; // optional :: G_BufferKeyNull()
};

typedef struct R_ObjectDesc R_ObjectDesc;
struct R_ObjectDesc
{
	m4 transform;
	v4 sphere_bounds;

	R_SceneHandle mesh;
	R_SceneHandle material;
};

typedef struct R_MaterialSlot R_MaterialSlot;
struct R_MaterialSlot
{
	R_Material source;
	u32 generation;
	b32 active;
};

typedef struct R_MeshSlot R_MeshSlot;
struct R_MeshSlot
{
	u32 page_index;

	u32 vertex_offset;
	u32 vertex_count;

	u32 index_offset;
	u32 index_count;
	
	u32 generation;
	b32 active;
};

typedef struct R_ObjectSlot R_ObjectSlot;
struct R_ObjectSlot
{
	m4 transform;
	v4 sphere_bounds;

	R_SceneHandle mesh;
	R_SceneHandle material;

	const m4 *skinning_palette;
	u32 skinning_joint_count;

	u64 skinning_palette_gpu_addr;

	u32 generation;
	b32 active;
};

typedef struct R_LightSlot R_LightSlot;
struct R_LightSlot
{
	R_Light light;

	u32 generation;
	b32 active;
};

typedef struct R_ShadowCaster R_ShadowCaster;
struct R_ShadowCaster
{
	v3 position;
	f32 far;
	f32 near;
	f32 radius;
};

typedef struct R_ModelImportEntry R_ModelImportEntry;
struct R_ModelImportEntry
{
	m4 transform;
	v4 sphere_bounds;

	R_SceneHandle mesh;
	R_SceneHandle material;

	i32 skin_index;
};

typedef struct R_ModelImportReceipt R_ModelImportReceipt;
struct R_ModelImportReceipt
{
	u32 count;
	R_ModelImportEntry *entries;
};

typedef struct R_SceneFrameData R_SceneFrameData;
struct R_SceneFrameData
{
	u32 page_count;
	
	G_Alloc object_buffer;
	G_Alloc light_buffer;
	G_Alloc page_table_buffer;
	G_Alloc skinning_palette_buffer;

	u32 shadow_caster_count;
	R_ShadowCaster *shadow_casters;
};

typedef struct R_Scene R_Scene;
struct R_Scene
{
	Arena           *arena;
	G_Device        *device;
	A_Registry      *assets;
	LOG_Channel      log_channel;

	R_ObjectSlot     object_slots[R_SCENE_MAX_OBJECTS];
	u32              object_count;
	u32              object_free_list[R_SCENE_MAX_OBJECTS];
	u32              object_free_count;

	R_LightSlot      light_slots[R_SCENE_MAX_LIGHTS];
	u32              light_count;
	u32              light_free_list[R_SCENE_MAX_LIGHTS];
	u32              light_free_count;

	R_ShadowCaster   shadow_casters[R_SCENE_MAX_SHADOW_CASTERS];
	u32              shadow_caster_count;

	R_GeometryPage   geometry_pages[R_SCENE_MAX_GEOMETRY_PAGES];
	u32              geometry_page_count;
	
	R_MaterialSlot   material_slots[R_SCENE_MAX_MATERIALS];
	R_GPU_Material   material_gpus[R_SCENE_MAX_MATERIALS];
	u32              material_count;
	u32              material_free_list[R_SCENE_MAX_MATERIALS];
	u32              material_free_count;
	G_BufferKey      material_buffer;
	b32              material_buffer_dirty;

	R_MeshSlot       mesh_slots[R_SCENE_MAX_MESHES];
	R_GPU_RenderMesh mesh_gpus[R_SCENE_MAX_MESHES];
	u32              mesh_count;
	u32              mesh_free_list[R_SCENE_MAX_MESHES];
	u32              mesh_free_count;
	G_BufferKey      mesh_buffer;
	b32              mesh_buffer_dirty;
};

// trying out new formatting :p

internal void                  R_SceneInit                  (      R_Scene *scene, Arena *arena, G_Device *device, A_Registry *assets, LOG_Channel log_channel);
internal void                  R_SceneDestroy               (      R_Scene *scene);

internal void                  R_SceneDrawIndirect          (const R_Scene *scene, G_CmdBuffer *cmd, G_BufferKey indirect_buffer, G_BufferKey count_buffer);

internal R_SceneFrameData      R_SceneUploadFrameData       (      R_Scene *scene, G_RingBuffer *ring);
internal void                  R_SceneUploadPageTable       (      R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);
internal void                  R_SceneUploadSkinning        (      R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);
internal void                  R_SceneUploadObjects         (      R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);
internal void                  R_SceneUploadLights          (      R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);

internal R_SceneHandle         R_SceneObjectCreate          (      R_Scene *scene, const R_ObjectDesc *desc);
internal void                  R_SceneObjectDestroy         (      R_Scene *scene, R_SceneHandle handle);
internal u32                   R_SceneObjectCount           (const R_Scene *scene);
internal R_ObjectSlot         *R_SceneObjectGetSlot         (      R_Scene *scene, R_SceneHandle handle);
internal void                  R_SceneObjectSetTransform    (      R_Scene *scene, R_SceneHandle handle, m4 transform);
internal void                  R_SceneObjectSetSphereBounds (      R_Scene *scene, R_SceneHandle handle, v4 sphere_bounds);
internal void                  R_SceneObjectSetMaterial     (      R_Scene *scene, R_SceneHandle handle, R_SceneHandle material);
internal void                  R_SceneObjectSetMesh         (      R_Scene *scene, R_SceneHandle handle, R_SceneHandle mesh);
internal void                  R_SceneObjectSetSkinning     (      R_Scene *scene, R_SceneHandle handle, const AN_Palette *palette);
internal b32                   R_SceneObjectHandleIsValid   (const R_Scene *scene, R_SceneHandle handle);

internal R_SceneHandle         R_SceneLightCreate           (      R_Scene *scene, const R_Light *light);
internal void                  R_SceneLightDestroy          (      R_Scene *scene, R_SceneHandle handle);
internal u32                   R_SceneLightCount            (const R_Scene *scene);
internal R_LightSlot          *R_SceneLightGetSlot          (      R_Scene *scene, R_SceneHandle handle);
internal void                  R_SceneLightSetPosition      (      R_Scene *scene, R_SceneHandle handle, v3 position);
internal void                  R_SceneLightSetColour        (      R_Scene *scene, R_SceneHandle handle, v3 colour);
internal void                  R_SceneLightSetIntensity     (      R_Scene *scene, R_SceneHandle handle, f32 intensity);
internal b32                   R_SceneLightHandleIsValid    (const R_Scene *scene, R_SceneHandle handle);

internal R_SceneHandle         R_SceneMaterialCreate        (      R_Scene *scene, const R_Material *material);
internal R_SceneHandle         R_SceneMaterialFromAssets    (      R_Scene *scene, const A_ModelMaterial *source);
internal void                  R_SceneMaterialUpdate        (      R_Scene *scene, R_SceneHandle handle, const R_Material *material);
internal void                  R_SceneMaterialDestroy       (      R_Scene *scene, R_SceneHandle handle);
internal u32                   R_SceneMaterialCount         (const R_Scene *scene);
internal const R_Material     *R_SceneMaterialGetSource     (const R_Scene *scene, R_SceneHandle handle);
internal u64                   R_SceneMaterialBufferAddr    (const R_Scene *scene);
internal void                  R_SceneMaterialBakeIntoGPU   (const R_Scene *scene, const R_Material *material, R_GPU_Material *out);
internal b32                   R_SceneMaterialHandleIsValid (const R_Scene *scene, R_SceneHandle handle);

internal R_SceneHandle         R_SceneMeshCreate            (      R_Scene *scene, G_CmdBuffer *cmd, const R_MeshDesc *desc);
internal void                  R_SceneMeshDestroy           (      R_Scene *scene, R_SceneHandle handle);
internal u32                   R_SceneMeshCount             (const R_Scene *scene);
internal u64                   R_SceneMeshBufferAddr        (const R_Scene *scene);
internal b32                   R_SceneMeshHandleIsValid     (const R_Scene *scene, R_SceneHandle handle);

internal void                  R_SceneFlushMaterialBuffer   (      R_Scene *scene);
internal void                  R_SceneFlushMeshBuffer       (      R_Scene *scene);

internal R_ModelImportReceipt  R_SceneImportModel           (      R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle handle, u32 max_count);

internal u32                   R_SceneFindSuitablePage      (      R_Scene *scene, u32 vertex_count, u32 index_count);
internal R_GeometryPage        R_SceneCreateNewPage         (      R_Scene *scene);
internal u32                   R_ScenePageCount             (const R_Scene *scene);

internal G_BindlessIndex       R_SceneResolveTextureKey     (const R_Scene *scene, G_TextureKey key);

#endif // RENDER_SCENE_H
