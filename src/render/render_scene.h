#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

typedef struct R_GeometryPage R_GeometryPage;
struct R_GeometryPage
{
	R_GeometryPage *next;
	
	const GFX_Buffer *vertex_buffer;
	const GFX_Buffer *index_buffer;

	u32 vertex_offset;
	u32 index_offset;

	u32 max_vertices;
	u32 max_indices;
};

typedef struct R_MeshMemoryLocation R_MeshMemoryLocation;
struct R_MeshMemoryLocation
{
	R_MeshMemoryLocation *next;
	
	u32 page;
	u32 index;
};

typedef struct R_SceneMesh R_SceneMesh;
struct R_SceneMesh
{
	R_SceneMesh *next;
};

typedef struct R_SceneMeshHandle R_SceneMeshHandle;
struct R_SceneMeshHandle
{
	u32 value;
};

typedef struct R_SceneMaterial R_SceneMaterial;
struct R_SceneMaterial
{
	R_SceneMaterial *next;
};

typedef struct R_SceneMaterialHandle R_SceneMaterialHandle;
struct R_SceneMaterialHandle
{
	u32 value;
};

typedef struct R_Object R_Object;
struct R_Object
{
	m4 transform;
	v4 sphere_bounds;
	R_SceneMeshHandle mesh;
	R_SceneMaterialHandle material;
};

typedef struct R_SceneObject R_SceneObject;
struct R_SceneObject
{
	R_SceneObject *next;
	
	R_Object object;
	u32 page_index;
};

typedef struct R_SceneObjectHandle R_SceneObjectHandle;
struct R_SceneObjectHandle
{
	u32 index;
	u32 generation;
};

typedef struct R_SceneLight R_SceneLight;
struct R_SceneLight
{
	R_SceneLight *next;
	
	R_Light light;
};

typedef struct R_SceneLightHandle R_SceneLightHandle;
struct R_SceneLightHandle
{
	u32 index;
	u32 generation;
};

typedef struct R_ShadowCaster R_ShadowCaster;
struct R_ShadowCaster
{
	R_ShadowCaster *next;
	
	v3 position;
	f32 near;
	f32 far;
	f32 radius;
};

typedef struct R_SceneResources R_SceneResources;
struct R_SceneResources
{
	GFX_Alloc object_buffer;
	GFX_Alloc light_buffer;
	GFX_Alloc page_table_buffer;
};

typedef struct R_Scene R_Scene;
struct R_Scene
{
	Arena *arena;

	R_SceneObject *object_head;
	R_SceneLight *light_head;
	
	R_ShadowCaster *shadow_caster_head;
	R_GeometryPage *geometry_page_head;
	
	R_MeshMemoryLocation *mesh_registry_head;

	R_SceneMesh *mesh_head;
	R_SceneMaterial *material_head;
	
	GFX_Buffer *mesh_buffer;
	GFX_Buffer *material_buffer;

	b32 mesh_buffer_dirty;
	b32 material_buffer_dirty;
};


/* ==================================================
   CORE
   ================================================== */

internal void R_SceneInit(R_Scene *scene, Arena *arena);
internal void R_SceneDestroy(R_Scene *scene);

internal void R_SceneDebug(const R_Scene *scene);

internal R_SceneResources R_SceneRefreshTransientResources(R_Scene *scene, GFX_RingBuffer *arena);

internal void R_SceneUpdateObjectBuffer (R_Scene *scene, GFX_RingBuffer *arena, R_SceneResources *out);
internal void R_SceneUpdateLightBuffer  (R_Scene *scene, GFX_RingBuffer *arena, R_SceneResources *out);
internal void R_SceneUpdatePageBuffer   (R_Scene *scene, GFX_RingBuffer *arena, R_SceneResources *out);

internal void R_SceneUpdateMaterialBuffer (R_Scene *scene);
internal void R_SceneUpdateMeshBuffer     (R_Scene *scene);

internal u32            R_SceneFindSuitablePage (const R_Scene *scene, u32 vertex_count, u32 index_bount);
internal R_GeometryPage R_SceneCreateNewPage    (const R_Scene *scene);


/* ==================================================
   OBJECTS
   ================================================== */

internal R_SceneObjectHandle R_SceneObjectCreate(R_Scene *scene, const R_Object *object);
internal void R_SceneObjectRemove(R_Scene *scene, R_SceneObjectHandle handle);

internal void R_SceneObjectSetTransform(R_Scene *scene, R_SceneObjectHandle handle, m4 transform);


/* =======================================================
   LIGHTS
   ======================================================= */

internal R_SceneLightHandle R_SceneLightCreate(R_Scene *scene, const R_Light *light);
internal void R_SceneLightDestroy(R_Scene *scene, R_SceneLightHandle handle);

internal void R_SceneLightSetPosition  (R_Scene *scene, R_SceneLightHandle handle, v3 position);
internal void R_SceneLightSetColour    (R_Scene *scene, R_SceneLightHandle handle, v3 colour);
internal void R_SceneLightSetIntensity (R_Scene *scene, R_SceneLightHandle handle, f32 intensity);


/* =======================================================
   MESHES & MATERIALS
   ======================================================= */

internal R_SceneMeshHandle     R_SceneRegisterMesh     (R_Scene *scene, const R_Mesh *mesh);
internal R_SceneMaterialHandle R_SceneRegisterMaterial (R_Scene *scene, const R_Material *material);


#endif // RENDER_SCENE_H
