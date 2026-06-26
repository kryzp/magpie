
#include "entity/entity_world.h"
#include "core/core_log.h"
static u32 E_PoolAllocSlot(E_TypePool *pool)
{
	if (pool->free_index_count > 0)
		return pool->free_indices[--pool->free_index_count];

	AssertTrue(pool->count < pool->capacity);

	return pool->count++;
}

static void E_PoolFreeSlot(E_TypePool *pool, u32 index)
{
	AssertTrue(pool->free_index_count < pool->capacity);

	pool->free_indices[pool->free_index_count] = index;
	pool->free_index_count++;
}

static void E_WorldInit(E_World *world, Arena *arena, LOG_Channel log_channel)
{
	world->arena = arena;
	world->log_channel = log_channel;
	
	world->next_uid = 1; // 0 is reserved as null invalid entity id.
	world->next_tid = 0;

	world->layers[0].active = true;
	world->layers[0].name = String8Lit("default");

	world->layer_count = 1;

	world->marker_count = 0;

	world->initting_entity_count = 0;

	DebugLogI(world->log_channel, "Initialized.");
}

static void E_WorldDestroy(E_World *world)
{
	for (u32 t = 0; t < world->next_tid; t++)
	{
		const E_TypeDesc *desc = &world->type_registry[t];

		if (!desc->OnDestroy)
			continue;
		
		E_TypePool *pool = &world->type_pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));
			
			desc->OnDestroy(entity);
		}
	}
	
	DebugLogI(world->log_channel, "Destroyed.");
}

static void E_WorldToggleLayer(E_World *world, u16 layer_id, b32 active)
{
	DebugLogAssert(world->log_channel, layer_id < world->layer_count, "Asked for layer %u, which exceeded max layer count of %u.", layer_id, world->layer_count);
	world->layers[layer_id].active = active;
}

static E_TID E_WorldRegisterType(E_World *world, const E_TypeDesc *desc)
{
	DebugLogAssert(
		world->log_channel,
		world->next_tid < E_WORLD_MAX_REGISTERED_TYPES,
		"Cannot register more entity types!"
	);

	u32 tid = world->next_tid;
	
	E_TypePool *pool = &world->type_pools[tid];

	pool->capacity = desc->max_instances;
	pool->count = 0;
	pool->free_index_count = 0;

	pool->data = ArenaPushArray(world->arena, u8,  desc->max_instances * desc->stride);
	pool->alive = ArenaPushArray(world->arena, b32, desc->max_instances);
	pool->free_indices = ArenaPushArray(world->arena, u32, desc->max_instances);
		
	world->type_registry[tid] = *desc;

	E_TID wrapped = { tid };

	world->next_tid++;

	return wrapped;
}

static void E_WorldResolveInittingEntities(const E_TickContext *ctx)
{
	for (u32 i = 0; i < ctx->world->initting_entity_count; i++)
	{
		const E_InittingEntity *initting_entity = &ctx->world->initting_entities[i];

		E_TID type = initting_entity->type;
		u32 slot = initting_entity->slot;
		
		const E_TypeDesc *desc = &ctx->world->type_registry[type.value];
		E_TypePool *pool = &ctx->world->type_pools[type.value];
	
		void *entity = (void *)(pool->data + (slot * desc->stride));

		MemZero(entity, desc->stride);

		pool->alive[slot] = true;

		E_Header *header = E_HeaderOf(entity);
		header->uid.value = ctx->world->next_uid;
		header->tid = type;
		header->flags = E_Flag_Active;
		header->layer_id = 0;
	
		if (desc->OnInit)
			desc->OnInit(entity, ctx);
	}

	ctx->world->initting_entity_count = 0;
}

static void E_WorldTickPreAnim(const E_TickContext *ctx)
{
	for (u32 t = 0; t < ctx->world->next_tid; t++)
	{
		const E_TypeDesc *desc = &ctx->world->type_registry[t];

		if (!desc->OnPreAnimTick)
			continue;

		E_TypePool *pool = &ctx->world->type_pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			E_Flags flags = E_HeaderOf(entity)->flags;
			u16 layer_id = E_HeaderOf(entity)->layer_id;
			
			if (!(flags & E_Flag_Active))
				continue;

			if (!ctx->world->layers[layer_id].active)
				continue;

			desc->OnPreAnimTick(entity, ctx);
		}
	}
}

static void E_WorldTickPostAnim(const E_TickContext *ctx)
{
	for (u32 t = 0; t < ctx->world->next_tid; t++)
	{
		const E_TypeDesc *desc = &ctx->world->type_registry[t];

		if (!desc->OnPostAnimTick)
			continue;

		E_TypePool *pool = &ctx->world->type_pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			E_Flags flags = E_HeaderOf(entity)->flags;
			u16 layer_id = E_HeaderOf(entity)->layer_id;
			
			if (!(flags & E_Flag_Active))
				continue;

			if (!ctx->world->layers[layer_id].active)
				continue;

			desc->OnPostAnimTick(entity, ctx);
		}
	}
}

static void E_WorldTickPostPhysics(const E_TickContext *ctx)
{
	for (u32 t = 0; t < ctx->world->next_tid; t++)
	{
		const E_TypeDesc *desc = &ctx->world->type_registry[t];

		if (!desc->OnPostPhysicsTick)
			continue;

		E_TypePool *pool = &ctx->world->type_pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			E_Flags flags = E_HeaderOf(entity)->flags;
			u16 layer_id = E_HeaderOf(entity)->layer_id;
			
			if (!(flags & E_Flag_Active))
				continue;

			if (!ctx->world->layers[layer_id].active)
				continue;

			desc->OnPostPhysicsTick(entity, ctx);
		}
	}
}

static void E_WorldFlush(E_World *world)
{
	for (u32 t = 0; t < world->next_tid; t++)
	{
		const E_TypeDesc *desc = &world->type_registry[t];

		if (!desc->OnPostAnimTick)
			continue;

		E_TypePool *pool = &world->type_pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			E_Flags flags = E_HeaderOf(entity)->flags;

			if (!(flags & E_Flag_PendingKill))
				continue;

			if (desc->OnDestroy)
				desc->OnDestroy(entity);

			MemZero(entity, desc->stride);

			pool->alive[j] = false;

			E_PoolFreeSlot(pool, j);
		}
	}
}

static E_Handle E_WorldSpawn(E_World *world, E_TID type, E_Transform transform)
{
	DebugLogAssert(
		world->log_channel,
		world->initting_entity_count < ArraySize(world->initting_entities),
		"Not enough space to init new entity this frame!"
	);
	
	E_TypePool *pool = &world->type_pools[type.value];

	world->initting_entities[world->initting_entity_count].type = type;
	world->initting_entities[world->initting_entity_count].slot = E_PoolAllocSlot(pool);
	
	world->initting_entity_count++;
	
	E_Handle handle = {0};
	handle.uid.value = world->next_uid;
	handle.tid.value = world->next_tid;
	handle.generation = 0;

	world->next_uid++;

	return handle;
}

static void E_WorldKill(E_World *world, E_Handle handle)
{
	void *entity = E_WorldGet(world, handle);

	if (entity)
	{
		E_Header *header = E_HeaderOf(entity);
		header->flags |= E_Flag_PendingKill;
	}
}

static b32 E_WorldHandleIsValid(E_World *world, E_Handle handle)
{
	return true;
}

static void *E_WorldGet(E_World *world, E_Handle handle)
{
	for (u32 t = 0; t < world->next_tid; t++)
	{
		const E_TypeDesc *desc = &world->type_registry[t];

		E_TypePool *pool = &world->type_pools[t];

		b8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			if (E_UIDMatch(E_HeaderOf(entity)->uid, handle.uid))
				return entity;
		}
	}

	DebugLogW(
		world->log_channel,
		"Couldn't find entity with UID: %u, TID: %u, GEN: %u",
		handle.uid.value,
		handle.tid.value,
		handle.generation
	);

	return NULL;
}

static E_GetAllReceipt E_WorldGetAll(E_World *world, E_TID type)
{
	E_TypePool *pool = &world->type_pools[type.value];

	E_GetAllReceipt receipt = {0};
	receipt.data = pool->data;
	receipt.count = pool->count;
	receipt.alive = pool->alive;
	receipt.stride = world->type_registry[type.value].stride;

	return receipt;
}

static E_Marker *E_WorldAddMarker(E_World *world, String8 name, v3 position, v4 rotation, u16 layer_id)
{
	DebugLogAssert(
		world->log_channel,
		world->marker_count < ArraySize(world->markers),
		"Ran out of available marker space."
	);
	
	E_Marker *m = &world->markers[world->marker_count];
	m->name = name;
	m->name_hash = HashStr8(name);
	m->position = position;
	m->rotation = rotation;
	m->layer_id = layer_id;
	
	world->marker_count++;
	
	return m;
}

static E_Marker *E_WorldFindMarker(E_World *world, String8 name)
{
	u64 hash = HashStr8(name);
	
	for (u32 i = 0; i < world->marker_count; i++)
	{
		if (world->markers[i].name_hash == hash)
			return &world->markers[i];
	}

	DebugLogW(world->log_channel, "Couldn't find marker with name %.*s", String8VArg(name));
	
	return NULL;
}
