#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H


/* ==================================================
   GEOMETRY PAGES
   ================================================== */

#define R_PAGE_VERTEX_BUFFER_SIZE  Megabytes(64)
#define R_PAGE_INDEX_BUFFER_SIZE   Megabytes(32)

typedef struct R_GeometryPage R_GeometryPage;
struct R_GeometryPage
{
	R_GeometryPage *next;
	
	GFX_BufferKey vertex_buffer;
	GFX_BufferKey index_buffer;

	u32 vertex_count;
	u32 index_count;

	u32 max_vertices;
	u32 max_indices;
};


/* ==================================================
   MESH REGISTRY
   ================================================== */

typedef struct R_MeshMemoryLocation R_MeshMemoryLocation;
struct R_MeshMemoryLocation
{
	u32 page;  // Index into geometry page of linked list.
	u32 index; // Index into meshes[] GPU data array.
};


/* ==================================================
   MESHES & MATERIALS
   ================================================== */

#define R_SCENE_MAX_MESHES     1024
#define R_SCENE_MAX_MATERIALS  512

typedef struct R_SceneMeshHandle R_SceneMeshHandle;
struct R_SceneMeshHandle
{
	u32 value;
};

typedef struct R_SceneMaterialHandle R_SceneMaterialHandle;
struct R_SceneMaterialHandle
{
	u32 value;
};


/* ==================================================
   OBJECTS
   ================================================== */

#define R_SCENE_MAX_OBJECTS 1024

typedef struct R_Object R_Object;
struct R_Object
{
	m4 transform;
	v4 sphere_bounds;
	R_SceneMeshHandle mesh;
	R_SceneMaterialHandle material;
	const m4 *skinning_palette;
	u32 skinning_bone_count;
};

typedef struct R_SceneObjectHandle R_SceneObjectHandle;
struct R_SceneObjectHandle
{
	u32 index;
	u32 generation;
};

typedef struct R_SceneObjectSlot R_SceneObjectSlot;
struct R_SceneObjectSlot
{
	R_Object object;
	u32 page_index;
	u32 generation;
	b32 active;

	u32 skinning_palette_address_this_frame;
};


/* ==================================================
   LIGHTS
   ================================================== */

#define R_SCENE_MAX_LIGHTS 256

typedef struct R_SceneLightHandle R_SceneLightHandle;
struct R_SceneLightHandle
{
	u32 index;
	u32 generation;
};

typedef struct R_SceneLightSlot R_SceneLightSlot;
struct R_SceneLightSlot
{
	R_Light light;
	u32 generation;
	b32 active;
};


/* ==================================================
   SHADOW CASTERS
   ================================================== */

#define R_SCENE_MAX_SHADOW_CASTERS 8

typedef struct R_ShadowCaster R_ShadowCaster;
struct R_ShadowCaster
{
	v3 position;
	f32 near;
	f32 far;
	f32 radius;
};


/* ==================================================
   TRANSIENT RESOURCES
   ================================================== */

// TODO: Give this a better name.
typedef struct R_SceneResources R_SceneResources;
struct R_SceneResources
{
	GFX_Alloc object_buffer;
	GFX_Alloc light_buffer;
	GFX_Alloc page_table_buffer;
	GFX_Alloc skinning_palette_buffer;
};


/* ==================================================
   SCENE
   ================================================== */

typedef struct R_Scene R_Scene;
struct R_Scene
{
	Arena *arena;
	GFX_Device *device;
	
	LOG_Channel log_channel;
	
	// Objects
	R_SceneObjectSlot object_slots[R_SCENE_MAX_OBJECTS];
	u32               object_count;
	u32               object_free_list[R_SCENE_MAX_OBJECTS];
	u32               object_free_count;

	// Lights
	R_SceneLightSlot light_slots[R_SCENE_MAX_LIGHTS];
	u32              light_count;
	u32              light_free_list[R_SCENE_MAX_LIGHTS];
	u32              light_free_count;

	// Shadow Casters
	R_ShadowCaster shadow_casters[R_SCENE_MAX_SHADOW_CASTERS];
	u32            shadow_caster_count;

	// Geometry Page
	R_GeometryPage *geometry_page_head;
	u32             geometry_page_count;

	// Mesh Registry
	R_MeshMemoryLocation mesh_registry[R_SCENE_MAX_MESHES];

	// Meshes
	R_GPU_RenderMesh gpu_meshes[R_SCENE_MAX_MESHES];
	u32              mesh_count;
	GFX_BufferKey    mesh_buffer;
	b32              mesh_buffer_dirty;

	// Materials
	R_GPU_Material gpu_materials[R_SCENE_MAX_MATERIALS];
	u32            material_count;
	GFX_BufferKey  material_buffer;
	b32            material_buffer_dirty;
};


/* ==================================================
   CORE
   ================================================== */

internal void R_SceneInit    (R_Scene *scene, Arena *arena, GFX_Device *device, LOG_Channel log_channel);
internal void R_SceneDestroy (R_Scene *scene);

internal void R_SceneDebug(const R_Scene *scene);

internal R_SceneResources R_SceneRefreshTransientResources(R_Scene *scene, GFX_RingBuffer *ring);

internal void R_SceneUpdateObjectBuffer   (R_Scene *scene, GFX_RingBuffer *ring, R_SceneResources *out);
internal void R_SceneUpdateLightBuffer    (R_Scene *scene, GFX_RingBuffer *ring, R_SceneResources *out);
internal void R_SceneUpdatePageBuffer     (R_Scene *scene, GFX_RingBuffer *ring, R_SceneResources *out);
internal void R_SceneUpdateSkinningBuffer (R_Scene *scene, GFX_RingBuffer *ring, R_SceneResources *out);

internal void R_SceneUpdateMeshBuffer     (R_Scene *scene);
internal void R_SceneUpdateMaterialBuffer (R_Scene *scene);

internal void R_SceneDrawIndirect (const R_Scene *scene,
								   GFX_CmdBuffer *cmd,
								   GFX_BufferKey indirect_buffer,
								   GFX_BufferKey count_buffer);

internal u32            R_SceneFindSuitablePage (R_Scene *scene, u32 vertex_count, u32 index_count);
internal R_GeometryPage R_SceneCreateNewPage    (R_Scene *scene);


/* ==================================================
   OBJECTS
   ================================================== */

internal R_SceneObjectSlot  *R_SceneObjectGetSlot (R_Scene *scene, R_SceneObjectHandle handle);

internal R_SceneObjectHandle R_SceneObjectCreate  (R_Scene *scene, const R_Object *object);
internal void                R_SceneObjectRemove  (R_Scene *scene, R_SceneObjectHandle handle);

internal void R_SceneObjectSetTransform(R_Scene *scene, R_SceneObjectHandle handle, m4 transform);

internal u32 R_SceneGetObjectCount(const R_Scene *scene);


/* =======================================================
   LIGHTS
   ======================================================= */

internal R_SceneLightSlot  *R_SceneLightGetSlot (R_Scene *scene, R_SceneLightHandle handle);

internal R_SceneLightHandle R_SceneLightCreate  (R_Scene *scene, const R_Light *light);
internal void               R_SceneLightRemove  (R_Scene *scene, R_SceneLightHandle handle);

internal void R_SceneLightSetPosition  (R_Scene *scene, R_SceneLightHandle handle, v3 position);
internal void R_SceneLightSetColour    (R_Scene *scene, R_SceneLightHandle handle, v3 colour);
internal void R_SceneLightSetIntensity (R_Scene *scene, R_SceneLightHandle handle, f32 intensity);

internal u32 R_SceneGetLightCount(const R_Scene *scene);


/* =======================================================
   SHADOW CASTER
   ======================================================= */

internal const R_ShadowCaster *R_SceneShadowCasterGet(const R_Scene *scene, u32 i);

internal u32 R_SceneGetShadowCasterCount(const R_Scene *scene);


/* =======================================================
   MESHES
   ======================================================= */

internal R_SceneMeshHandle R_SceneRegisterMeshFromBuffers(R_Scene *scene,
														  const GFX_CmdBuffer *cmd,
														  GFX_BufferKey vertex_buffer,
														  GFX_BufferKey index_buffer,
														  u32 vertex_count,
														  u32 index_count,
														  GFX_BufferKey skin_buffer);

internal R_SceneMeshHandle R_SceneRegisterMesh(R_Scene *scene, const R_Mesh *mesh);

internal u64 R_SceneMeshBufferAddress(const R_Scene *scene);


/* =======================================================
   MODELS
   ======================================================= */

typedef struct R_SceneModelEntry R_SceneModelEntry;
struct R_SceneModelEntry
{
	R_SceneMeshHandle mesh;
	R_SceneMaterialHandle material;
	m4 transform;
	v4 sphere_bounds;
};

typedef struct R_SceneRegisterModelReceipt R_SceneRegisterModelReceipt;
struct R_SceneRegisterModelReceipt
{
	u32 entry_count;
	R_SceneModelEntry *entries;
};

internal R_SceneRegisterModelReceipt R_SceneRegisterModel(R_Scene *scene,
														  Arena *arena,
														  AST_Assets *assets,
														  AST_Handle model_handle,
														  u32 max_entries);


/* =======================================================
   MATERIALS
   ======================================================= */

internal u32 R_SceneResolveTextureBindless(const R_Scene *scene,
										   AST_Assets *assets,
										   AST_Handle handle);

internal R_SceneMaterialHandle R_SceneRegisterMaterial (R_Scene *scene,
														const AST_ModelMaterial *material,
														AST_Assets *assets);

internal u64 R_SceneMaterialBufferAddress(const R_Scene *scene);


#endif // RENDER_SCENE_H
