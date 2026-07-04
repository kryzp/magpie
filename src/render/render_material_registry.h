#ifndef RENDER_MATERIAL_REGISTRY_H
#define RENDER_MATERIAL_REGISTRY_H

#define R_MATERIAL_REGISTRY_MAX_MATERIALS 1024

typedef struct R_MaterialSlot R_MaterialSlot;
struct R_MaterialSlot
{
	R_Material source;
	u32 generation;
	b32 active;
};

typedef struct R_MaterialRegistry R_MaterialRegistry;
struct R_MaterialRegistry
{
	G_Device *device;
	A_Assets *assets;
	LOG_Channel log_channel;
	
	R_MaterialSlot material_slots[R_MATERIAL_REGISTRY_MAX_MATERIALS];
	R_GPU_Material material_gpus[R_MATERIAL_REGISTRY_MAX_MATERIALS];
	u32 material_count;
	u32 material_free_list[R_MATERIAL_REGISTRY_MAX_MATERIALS];
	u32 material_free_count;
	G_BufferKey material_buffer;
	b32 material_buffer_dirty;
};

static void                  R_MaterialRegistryInit(R_MaterialRegistry *r, G_Device *device, A_Assets *assets, LOG_Channel log_channel);
static void                  R_MaterialRegistryDestroy(R_MaterialRegistry *r);

static R_SceneHandle         R_MaterialRegistryAddMaterial(R_MaterialRegistry *r, const R_Material *material);
static R_SceneHandle         R_MaterialRegistryAddFromAssets(R_MaterialRegistry *r, const A_ModelMaterial *source);

static void                  R_MaterialRegistryUpdate(R_MaterialRegistry *r, R_SceneHandle handle, const R_Material *material);
static void                  R_MaterialRegistryDisposeOfMaterial(R_MaterialRegistry *r, R_SceneHandle handle);

static u32                   R_MaterialRegistryCountOfMaterials(const R_MaterialRegistry *r);
static const R_Material     *R_MaterialRegistryGetSource(const R_MaterialRegistry *r, R_SceneHandle handle);
static u64                   R_MaterialRegistryBufferAddr(const R_MaterialRegistry *r);
static void                  R_MaterialRegistryBakeIntoGPU(const R_MaterialRegistry *r, const R_Material *material, R_GPU_Material *out);
static b32                   R_MaterialRegistryHandleIsValid(const R_MaterialRegistry *r, R_SceneHandle handle);

static void                  R_MaterialRegistryFlushIfDirty(R_MaterialRegistry *r);

static G_BindlessIndex       R_MaterialRegistryResolveToBindless(const R_MaterialRegistry *r, G_TextureKey key);

#endif // RENDER_MATERIAL_REGISTRY_H
