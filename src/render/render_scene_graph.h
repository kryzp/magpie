#ifndef RENDER_SCENE_GRAPH_H
#define RENDER_SCENE_GRAPH_H

#define R_SCENE_GRAPH_MAX_OBJECTS            1024
#define R_SCENE_GRAPH_MAX_LIGHTS              128

typedef struct R_ObjectDesc R_ObjectDesc;
struct R_ObjectDesc
{
	m4 transform;
	v4 sphere_bounds;

	R_SceneHandle mesh;
	R_SceneHandle material;
};

typedef struct R_ObjectSlot R_ObjectSlot;
struct R_ObjectSlot
{
	m4 transform;
	m4 normal_matrix;

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

typedef struct R_SceneGraph R_SceneGraph;
struct R_SceneGraph
{
	LOG_Channel log_channel;

	R_ObjectSlot object_slots[R_SCENE_GRAPH_MAX_OBJECTS];
	u32 object_count;
	u32 object_free_list[R_SCENE_GRAPH_MAX_OBJECTS];
	u32 object_free_count;

	R_LightSlot light_slots[R_SCENE_GRAPH_MAX_LIGHTS];
	u32 light_count;
	u32 light_free_list[R_SCENE_GRAPH_MAX_LIGHTS];
	u32 light_free_count;
};

static void                  R_SceneGraphInit(R_SceneGraph *sg, LOG_Channel log_channel);
static void                  R_SceneGraphDestroy(R_SceneGraph *sg);

static R_SceneHandle         R_SceneGraphObjectCreate(R_SceneGraph *sg, const R_ObjectDesc *desc);
static void                  R_SceneGraphObjectDestroy(R_SceneGraph *sg, R_SceneHandle handle);
static u32                   R_SceneGraphObjectCount(const R_SceneGraph *sg);
static R_ObjectSlot         *R_SceneGraphObjectGetSlot(R_SceneGraph *sg, R_SceneHandle handle);
static void                  R_SceneGraphObjectSetTransform(R_SceneGraph *sg, R_SceneHandle handle, m4 transform);
static void                  R_SceneGraphObjectSetSphereBounds(R_SceneGraph *sg, R_SceneHandle handle, v4 sphere_bounds);
static void                  R_SceneGraphObjectSetMaterial(R_SceneGraph *sg, R_SceneHandle handle, R_SceneHandle material);
static void                  R_SceneGraphObjectSetMesh(R_SceneGraph *sg, R_SceneHandle handle, R_SceneHandle mesh);
static void                  R_SceneGraphObjectSetSkinning(R_SceneGraph *sg, R_SceneHandle handle, const AN_Palette *palette);
static b32                   R_SceneGraphObjectHandleIsValid(const R_SceneGraph *sg, R_SceneHandle handle);

static R_SceneHandle         R_SceneGraphLightCreate(R_SceneGraph *sg, const R_Light *light);
static void                  R_SceneGraphLightDestroy(R_SceneGraph *sg, R_SceneHandle handle);
static u32                   R_SceneGraphLightCount(const R_SceneGraph *sg);
static R_LightSlot          *R_SceneGraphLightGetSlot(R_SceneGraph *sg, R_SceneHandle handle);
static void                  R_SceneGraphLightSetPosition(R_SceneGraph *sg, R_SceneHandle handle, v3 position);
static void                  R_SceneGraphLightSetColour(R_SceneGraph *sg, R_SceneHandle handle, v3 colour);
static void                  R_SceneGraphLightSetIntensity(R_SceneGraph *sg, R_SceneHandle handle, f32 intensity);
static b32                   R_SceneGraphLightHandleIsValid(const R_SceneGraph *sg, R_SceneHandle handle);

#endif // RENDER_SCENE_GRAPH_H
