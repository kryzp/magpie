#ifndef ENTITY_WORLD_H
#define ENTITY_WORLD_H

#define ENT_WORLD_MAX_SCENE_LAYERS 16

typedef struct ENT_TypePool ENT_TypePool;
struct ENT_TypePool
{
	void *data; // array <stride * max_instances>
	u32 num_alive;
	u32 capacity;
	b32 *alive;
};

typedef struct ENT_Locator ENT_Locator;
struct ENT_Locator
{
	ENT_Type type;
	u32 slot_index;
};

typedef struct ENT_World ENT_World;
struct ENT_World
{
	ENT_TypePool pools[ENT_Type_COUNT];

	ENT_Locator *id_map;
	u32 next_entity_id;

	ENT_SceneLayer layers[ENT_WORLD_MAX_SCENE_LAYERS];
	u32 layer_count;

	ENT_EventQueue events;
};


/* ==================================================
   CORE
   ================================================== */

internal void ENT_WorldInit    (ENT_World *world);
internal void ENT_WorldDestroy (ENT_World *world);


/* ==================================================
   ENTITIES
   ================================================== */

internal void *ENT_WorldSpawn (ENT_World *world, ENT_Type type);
internal void *ENT_WorldKill  (ENT_World *world, ENT_UID uid);


/* ==================================================
   LOOKUP
   ================================================== */

internal void *ENT_WorldGet(ENT_World *world, ENT_UID uid);

#define ENT_WorldGetHeader(world, uid) ((ENT_Header *)ENT_WorldGet((world), (uid)))


typedef struct ENT_GetAllReceipt ENT_GetAllReceipt;
struct ENT_GetAllReceipt
{
	void *elements;
	u64 count;
};

internal ENT_GetAllReceipt ENT_WorldGetAll(ENT_World *world, ENT_Type type);


/* ==================================================
   PER-FRAME
   ================================================== */

void ENT_WorldTick(ENT_World *world, f32 dt);
void ENT_WorldFlush(ENT_World *world); // Clean-up all entities marked for pending kill.


#endif // ENTITY_WORLD_H
