#ifndef ENTITY_WORLD_H
#define ENTITY_WORLD_H

#define E_WORLD_MAX_SCENE_LAYERS      16
#define E_WORLD_MAX_MARKERS           512
#define E_MAX_NEW_ENTITIES_PER_FRAME  512
#define E_WORLD_MAX_REGISTERED_TYPES  512

typedef struct E_TypePool E_TypePool;
struct E_TypePool
{
	u8 *data;
	b32 *alive;

	u32 count;
	u32 capacity;

	u32 *free_indices;
	u32 free_index_count;
};

/*
 * Markers are just useful ways to quickly "mark"
 * points in the world ahead of time for use in
 * code.
 */
typedef struct E_Marker E_Marker;
struct E_Marker
{
	String8 name;
	u64 name_hash;
	v3 position;
	v4 rotation;
	u16 layer_id;
};

typedef struct E_InittingEntity E_InittingEntity;
struct E_InittingEntity
{
	E_TID type;
	u32 slot;
};

typedef struct E_World E_World;
struct E_World
{
	Arena *arena;
	LOG_Channel log_channel;

	AU_System *audio;
	A_Assets *assets;
	AN_System *animation;
	P_Engine *physics;
	
	u32 next_uid;
	u32 next_tid;

	E_TypePool type_pools[E_WORLD_MAX_REGISTERED_TYPES];
	E_TypeDesc type_registry[E_WORLD_MAX_REGISTERED_TYPES];

	E_SceneLayer layers[E_WORLD_MAX_SCENE_LAYERS];
	u32 layer_count;

	E_Marker markers[E_WORLD_MAX_MARKERS];
	u32 marker_count;

	u32 initting_entity_count;
	E_InittingEntity initting_entities[E_MAX_NEW_ENTITIES_PER_FRAME];
};


/* ==================================================
   HELPERS
   ================================================== */

static u32  E_PoolAllocSlot (E_TypePool *pool);
static void E_PoolFreeSlot  (E_TypePool *pool, u32 index);


/* ==================================================
   CORE
   ================================================== */

static void E_WorldInit(E_World *world, Arena *arena, LOG_Channel log_channel);
static void E_WorldDestroy(E_World *world);

static void E_WorldToggleLayer(E_World *world, u16 layer_id, b32 active);
static E_TID E_WorldRegisterType(E_World *world, const E_TypeDesc *desc);


/* ==================================================
   PER-FRAME
   ================================================== */

static void E_WorldResolveInittingEntities(const E_TickContext *ctx);
static void E_WorldTickPreAnim(const E_TickContext *ctx);
static void E_WorldTickPostAnim(const E_TickContext *ctx);
static void E_WorldTickPostPhysics(const E_TickContext *ctx);

static void E_WorldFlush(E_World *world); // Clean-up all entities marked for pending kill.


/* ==================================================
   ENTITIES
   ================================================== */

static E_Handle E_WorldSpawn(E_World *world, E_TID type, E_Transform transform);
static void E_WorldKill(E_World *world, E_Handle handle);
static b32 E_WorldHandleIsValid(E_World *world, E_Handle handle);


/* ==================================================
   LOOKUP
   ================================================== */

static void *E_WorldGet(E_World *world, E_Handle handle);


typedef struct E_GetAllReceipt E_GetAllReceipt;
struct E_GetAllReceipt
{
	u8 *data;
	u32 count;
	b32 *alive;
	u32 stride;
};

static E_GetAllReceipt E_WorldGetAll(E_World *world, E_TID type);


/* ==================================================
   MARKERS
   ================================================== */

static E_Marker *E_WorldAddMarker  (E_World *world, String8 name, v3 position, v4 rotation, u16 layer_id);
static E_Marker *E_WorldFindMarker (E_World *world, String8 name);


#endif // ENTITY_WORLD_H
