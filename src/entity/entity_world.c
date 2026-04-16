
internal u32
ENT_PoolAllocSlot(ENT_TypePool *pool)
{
	if (pool->free_index_count > 0)
		return pool->free_indices[--pool->free_index_count];

	AssertTrue(pool->count < pool->capacity);
	return pool->count++;
}

internal void
ENT_PoolFreeSlot(ENT_TypePool *pool, u32 index)
{
	AssertTrue(pool->free_index_count < pool->capacity);

	pool->free_indices[pool->free_index_count] = index;
	pool->free_index_count++;
}

internal void
ENT_WorldInit(ENT_World *world, Arena *arena)
{
	MemZeroStruct(world);

	world->arena = arena;
	world->next_uid = 1; // 0 is reserved as null invalid entity id.

	for (u32 t = 0; t < ENT_Type_COUNT; t++)
	{
		const ENT_TypeDesc *desc = &ent_global_types[t];

		ENT_TypePool *pool = &world->pools[t];

		pool->capacity = desc->max_instances;
		pool->count = 0;
		pool->free_index_count = 0;

		pool->data         = ArenaPushArray(arena, u8,  desc->max_instances * desc->stride);
		pool->alive        = ArenaPushArray(arena, b32, desc->max_instances);
		pool->free_indices = ArenaPushArray(arena, u32, desc->max_instances);
	}

	world->layers[0].active = true;
	world->layers[0].name = Str8("default");

	world->layer_count = 1;
}

internal void
ENT_WorldDestroy(ENT_World *world)
{
	for (u32 t = 0; t < ENT_Type_COUNT; t++)
	{
		const ENT_TypeDesc *desc = &ent_global_types[t];

		if (!desc->OnDestroy)
			continue;
		
		ENT_TypePool *pool = &world->pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));
			
			desc->OnDestroy(entity);
		}
	}
}

internal void
ENT_WorldToggleLayer(ENT_World *world, u16 layer_id, b32 active)
{
	AssertTrue(layer_id < world->layer_count);
	world->layers[layer_id].active = active;
}

internal void
ENT_WorldTick(ENT_World *world, ENT_EventQueue *events, f32 dt, const I_InputSt *input)
{
	ENT_TickContext ctx = {0};
	ctx.world = world;
	ctx.events = events;
	ctx.dt = dt;
	ctx.input = input;
	
	for (u32 t = 0; t < ENT_Type_COUNT; t++)
	{
		const ENT_TypeDesc *desc = &ent_global_types[t];

		if (!desc->OnPreAnimTick)
			continue;

		ENT_TypePool *pool = &world->pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			ENT_Flags flags = ENT_FlagsOf(entity);
			u16 layer_id = ENT_LayerIDOf(entity);
			
			if (!(flags & ENT_Flag_Active))
				continue;

			if (!world->layers[layer_id].active)
				continue;

			desc->OnPreAnimTick(entity, &ctx);
		}
	}

	// TODO: anim update

	for (u32 t = 0; t < ENT_Type_COUNT; t++)
	{
		const ENT_TypeDesc *desc = &ent_global_types[t];

		if (!desc->OnPostAnimTick)
			continue;

		ENT_TypePool *pool = &world->pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			ENT_Flags flags = ENT_FlagsOf(entity);
			u16 layer_id = ENT_LayerIDOf(entity);
			
			if (!(flags & ENT_Flag_Active))
				continue;

			if (!world->layers[layer_id].active)
				continue;

			desc->OnPostAnimTick(entity, &ctx);
		}
	}

	// TODO: physics step
	// TODO: should be in the fixed update!!

	for (u32 t = 0; t < ENT_Type_COUNT; t++)
	{
		const ENT_TypeDesc *desc = &ent_global_types[t];

		if (!desc->OnPostPhysicsTick)
			continue;

		ENT_TypePool *pool = &world->pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			ENT_Flags flags = ENT_FlagsOf(entity);
			u16 layer_id = ENT_LayerIDOf(entity);
			
			if (!(flags & ENT_Flag_Active))
				continue;

			if (!world->layers[layer_id].active)
				continue;

			desc->OnPostPhysicsTick(entity, &ctx);
		}
	}
}

internal void
ENT_WorldFlush(ENT_World *world)
{
	for (u32 t = 0; t < ENT_Type_COUNT; t++)
	{
		const ENT_TypeDesc *desc = &ent_global_types[t];

		if (!desc->OnPostAnimTick)
			continue;

		ENT_TypePool *pool = &world->pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			ENT_Flags flags = ENT_FlagsOf(entity);

			if (!(flags & ENT_Flag_PendingKill))
				continue;

			if (desc->OnDestroy)
				desc->OnDestroy(entity);

			MemZero(entity, desc->stride);

			pool->alive[j] = false;

			ENT_PoolFreeSlot(pool, j);
		}
	}
}

internal void *
ENT_WorldSpawn(ENT_World *world, ENT_Type type)
{
	const ENT_TypeDesc *desc = &ent_global_types[type];
	ENT_TypePool *pool = &world->pools[type];

	u32 slot = ENT_PoolAllocSlot(pool);

	void *entity = (void *)(pool->data + (slot * desc->stride));

	MemZero(entity, desc->stride);

	pool->alive[slot] = true;

	ENT_Header *header = ENT_HeaderOf(entity);
	header->uid.value = world->next_uid;
	header->type = type;
	header->flags = ENT_Flag_Active | ENT_Flag_Visible;
	header->layer_id = 0;

	ENT_TransformSetPosition (&header->transform, v3x(0.f));
	ENT_TransformSetRotation (&header->transform, V4QuatIdentity());
	ENT_TransformSetScale    (&header->transform, v3x(1.f));
	ENT_TransformSetOrigin   (&header->transform, v3x(0.f));
	
	ENT_TransformRecompute(&header->transform);
	
	world->next_uid++;

	return entity;
}

internal void
ENT_WorldKill(ENT_World *world, ENT_UID uid)
{
	void *entity = ENT_WorldGet(world, uid);

	if (entity)
	{
		ENT_Header *header = ENT_HeaderOf(entity);
		header->flags |= ENT_Flag_PendingKill;
	}
}

internal void *
ENT_WorldGet(ENT_World *world, ENT_UID uid)
{
	for (u32 t = 0; t < ENT_Type_COUNT; t++)
	{
		const ENT_TypeDesc *desc = &ent_global_types[t];

		ENT_TypePool *pool = &world->pools[t];

		b8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			if (ENT_UIDMatch(ENT_UIDOf(entity), uid))
				return entity;
		}
	}

	return NULL;
}

internal ENT_GetAllReceipt
ENT_WorldGetAll(ENT_World *world, ENT_Type type)
{
	ENT_TypePool *pool = &world->pools[type];

	ENT_GetAllReceipt receipt = {0};
	receipt.data = pool->data;
	receipt.count = pool->count;
	receipt.alive = pool->alive;
	receipt.stride = ent_global_types[type].stride;

	return receipt;
}
