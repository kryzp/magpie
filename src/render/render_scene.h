#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

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

typedef struct R_Model R_Model;
struct R_Model
{
	A_Handle asset_handle;
	AN_Handle animator_handle;
	u32 object_count;
	R_SceneHandle *objects;
	m4 *local_transforms;
	i32 *skin_indices;
};

typedef struct R_Scene R_Scene;
struct R_Scene
{
	LOG_Channel log_channel;

	R_SceneGraph graph;
	R_MeshRegistry meshes;
	R_MaterialRegistry materials;
};

// trying out new formatting :p

static void R_SceneInit(R_Scene *scene, Arena *arena, LOG_Channel log_channel);
static void R_SceneDestroy(R_Scene *scene);

static R_ModelImportReceipt R_SceneImportModel(R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle asset_handle);

static R_Model R_SceneModelCreate(R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle asset_handle, b32 animated);
static void R_SceneModelDestroy(R_Scene *scene, R_Model *model);
static void R_SceneModelSetRootTransform(R_Scene *scene, R_Model *model, m4 root_transform);
static void R_SceneModelUpdateSkinning(R_Scene *scene, R_Model *model);

#endif // RENDER_SCENE_H
