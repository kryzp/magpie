#ifndef ENTITY_WORLD_H
#define ENTITY_WORLD_H

#define ENT_WORLD_MAX_SCENE_LAYERS 16

typedef struct ENT_TypePool ENT_TypePool;
struct ENT_TypePool
{
	u8 *data;
	b32 *alive;

	u32 count;
	u32 capacity;

	u32 *free_indices;
	u32 free_index_count;
};

/*
typedef struct ENT_Locator ENT_Locator;
struct ENT_Locator
{
	ENT_Type type;
	u32 slot_index;
};
*/

typedef struct ENT_World ENT_World;
struct ENT_World
{
	Arena *arena;
	
	ENT_TypePool pools[ENT_Type_COUNT];

	ENT_SceneLayer layers[ENT_WORLD_MAX_SCENE_LAYERS];
	u32 layer_count;

	u32 next_uid;
};


/* ==================================================
   HELPERS
   ================================================== */

internal u32  ENT_PoolAllocSlot (ENT_TypePool *pool);
internal void ENT_PoolFreeSlot  (ENT_TypePool *pool, u32 index);


/* ==================================================
   CORE
   ================================================== */

internal void ENT_WorldInit    (ENT_World *world, Arena *arena);
internal void ENT_WorldDestroy (ENT_World *world);

internal void ENT_WorldToggleLayer(ENT_World *world, u16 layer_id, b32 active);


/* ==================================================
   PER-FRAME
   ================================================== */

internal void ENT_WorldTickPreAnim     (ENT_World *world, ENT_EventQueue *events, f32 dt, const I_State *input);
internal void ENT_WorldTickPostAnim    (ENT_World *world, ENT_EventQueue *events, f32 dt, const I_State *input);
internal void ENT_WorldTickPostPhysics (ENT_World *world, ENT_EventQueue *events, f32 dt, const I_State *input);

internal void ENT_WorldFlush(ENT_World *world); // Clean-up all entities marked for pending kill.


/* ==================================================
   ENTITIES
   ================================================== */

internal void *ENT_WorldSpawn (ENT_World *world, ENT_Type type);
internal void  ENT_WorldKill  (ENT_World *world, ENT_UID uid);


/* ==================================================
   LOOKUP
   ================================================== */

internal void *ENT_WorldGet(ENT_World *world, ENT_UID uid);

#define ENT_WorldGetHeader(world, uid) ENT_HeaderOf(ENT_WorldGet((world, (uid))))


typedef struct ENT_GetAllReceipt ENT_GetAllReceipt;
struct ENT_GetAllReceipt
{
	u8 *data;
	u32 count;
	b32 *alive;
	u32 stride;
};

internal ENT_GetAllReceipt ENT_WorldGetAll(ENT_World *world, ENT_Type type);


#endif // ENTITY_WORLD_H
