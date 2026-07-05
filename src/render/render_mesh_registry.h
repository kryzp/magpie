#ifndef RENDER_MESH_REGISTRY_H
#define RENDER_MESH_REGISTRY_H

#define R_MESH_REGISTRY_MAX_MESHES           1024
#define R_MESH_REGISTRY_MAX_GEOMETRY_PAGES     16

typedef struct R_MeshDesc R_MeshDesc;
struct R_MeshDesc
{
	G_BufferKey vertex_buffer;
	G_BufferKey index_buffer;

	u32 vertex_count;
	u32 index_count;

	G_BufferKey skin_buffer; // optional :: G_BufferKeyNull()
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

typedef struct R_MeshRegistry R_MeshRegistry;
struct R_MeshRegistry
{
	Arena *arena;
	LOG_Channel log_channel;
	
	R_MeshSlot mesh_slots[R_MESH_REGISTRY_MAX_MESHES];
	R_GPU_RenderMesh mesh_gpus[R_MESH_REGISTRY_MAX_MESHES];
	u32 mesh_count;
	u32 mesh_free_list[R_MESH_REGISTRY_MAX_MESHES];
	u32 mesh_free_count;
	G_BufferKey mesh_buffer;
	b32 mesh_buffer_dirty;

	R_GeometryPage geometry_pages[R_MESH_REGISTRY_MAX_GEOMETRY_PAGES];
	u32 geometry_page_count;
};

static void                 R_MeshRegistryInit(R_MeshRegistry *r, Arena *arena, LOG_Channel log_channel);
static void                 R_MeshRegistryDestroy(R_MeshRegistry *r);

static R_SceneHandle        R_MeshRegistryCreateMesh(R_MeshRegistry *r, G_CmdBuffer *cmd, const R_MeshDesc *desc);
static void                 R_MeshRegistryDestroyMesh(R_MeshRegistry *r, R_SceneHandle handle);

static u32                  R_MeshRegistryCountOfMeshes(const R_MeshRegistry *r);
static b32                  R_MeshRegistryHandleIsValid(const R_MeshRegistry *r, R_SceneHandle handle);

static void                 R_MeshRegistryFlushIfDirty(R_MeshRegistry *r);

static u32                  R_MeshRegistryFindSuitablePage(R_MeshRegistry *r, u32 vertex_count, u32 index_count);
static R_GeometryPage       R_MeshRegistryCreateNewPage(R_MeshRegistry *r);
static u32                  R_MeshRegistryPageCount(const R_MeshRegistry *r);

#endif // RENDER_MESH_REGISTRY_H
