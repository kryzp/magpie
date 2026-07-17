#ifndef RENDER_MODEL_H
#define RENDER_MODEL_H

typedef struct R_SubModel R_SubModel;
struct R_SubModel
{
	R_SubModel *next;
	
	R_MeshHandle mesh;
	R_MaterialHandle material;

	m4 local_transform; // relative to root
	v4 local_sphere_bounds;

	i32 skin_index;
};

typedef struct R_Model R_Model;
struct R_Model
{
	A_Handle asset_handle;
	R_SubModel *submodel_first;
};

typedef struct R_SubModelInstance R_SubModelInstance;
struct R_SubModelInstance
{
	R_SubModelInstance *next;
	R_InstanceHandle handle;
};

typedef struct R_ModelInstance R_ModelInstance;
struct R_ModelInstance
{
	A_Handle asset_handle;
	R_SubModelInstance *submodel_first;
	m4 root_transform;
	b32 has_animator;
	AN_Handle animator_handle;
};

typedef struct R_ModelCatalogueEntry R_ModelCatalogueEntry;
struct R_ModelCatalogueEntry
{
	R_ModelCatalogueEntry *next;
	R_ModelCatalogueEntry *prev;
	
	A_Handle asset_handle;
	R_Model model;
	u32 ref_count;
};

typedef struct R_ModelCatalogue R_ModelCatalogue;
struct R_ModelCatalogue
{
	Arena *arena;
	R_Scene *equipped_scene;

	R_ModelCatalogueEntry entry_first_sentinel;
	R_ModelCatalogueEntry first_free_entry_sentinel;
};


/* ==================================================
   UTILS
   ================================================== */

static R_Model R_ModelFromAsset(Arena *arena, R_Scene *scene, A_Handle asset_handle);
static R_ModelCatalogueEntry *R_ModelCatalogueTryFindEntry(R_ModelCatalogue *catalogue, A_Handle asset_handle);


/* ==================================================
   CORE
   ================================================== */

static void R_ModelCatalogueInit(R_ModelCatalogue *catalogue, Arena *arena);
static void R_ModelCatalogueEquipScene(R_ModelCatalogue *catalogue, R_Scene *scene);
static void R_ModelCatalogueDestroy(R_ModelCatalogue *catalogue);

static R_Model *R_ModelCatalogueCreateModel(R_ModelCatalogue *catalogue, A_Handle asset_handle);
static void R_ModelCatalogueReleaseModel(R_ModelCatalogue *catalogue, A_Handle asset_handle);

static R_Model *R_ModelCatalogueTryFindModel(R_ModelCatalogue *catalogue, A_Handle asset_handle);


/* ==================================================
   INSTANCES
   ================================================== */

static R_ModelInstance R_ModelInstanceCreate(R_ModelCatalogue *catalogue, A_Handle asset_handle, m4 initial_transform);
static R_ModelInstance R_ModelInstanceCreateFromPath(R_ModelCatalogue *catalogue, String8 asset_path, m4 initial_transform);

static void R_ModelInstanceDestroy(R_ModelCatalogue *catalogue, R_ModelInstance *instance);

static void R_ModelInstanceSetTransform(R_ModelCatalogue *catalogue, R_ModelInstance *instance, m4 root_transform);

static v4 R_ModelPartWorldSphereBounds(m4 world_transform, v4 local_sphere_bounds);


#endif // RENDER_MODEL_H
