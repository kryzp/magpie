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

typedef struct R_Scene R_Scene;
struct R_Scene
{
	A_Assets *assets;
	LOG_Channel log_channel;

	R_SceneGraph graph;
	R_MeshRegistry meshes;
	R_MaterialRegistry materials;
};

// trying out new formatting :p

static void R_SceneInit(R_Scene *scene, Arena *arena, G_Device *device, A_Assets *assets, LOG_Channel log_channel);
static void R_SceneDestroy(R_Scene *scene);

static void R_SceneDrawIndirect(const R_Scene *scene, G_CmdBuffer *cmd, G_BufferKey indirect_buffer, G_BufferKey count_buffer);

static R_ModelImportReceipt R_SceneImportModel(R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle handle, u32 max_count);

#endif // RENDER_SCENE_H
