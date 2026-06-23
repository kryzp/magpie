
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
	MemZeroStruct(world);

	world->arena = arena;
	world->log_channel = log_channel;
	
	world->next_uid = 1; // 0 is reserved as null invalid entity id.

	world->layers[0].active = true;
	world->layers[0].name = String8Lit("default");

	world->layer_count = 1;

	DebugLogI(world->log_channel, "World Initialized.");
}

static void E_WorldDestroy(E_World *world)
{
	for (u32 t = 0; t < E_Type_COUNT; t++)
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
	
	DebugLogI(world->log_channel, "World Destroyed.");
}

static void E_WorldToggleLayer(E_World *world, u16 layer_id, b32 active)
{
	AssertTrue(layer_id < world->layer_count);
	world->layers[layer_id].active = active;
}

static void E_WorldRegisterType(E_World *world, const E_TypeDesc *desc)
{
	E_TypePool *pool = &world->type_pools[desc->type];

	pool->capacity = desc->max_instances;
	pool->count = 0;
	pool->free_index_count = 0;

	pool->data         = ArenaPushArray(world->arena, u8,  desc->max_instances * desc->stride);
	pool->alive        = ArenaPushArray(world->arena, b32, desc->max_instances);
	pool->free_indices = ArenaPushArray(world->arena, u32, desc->max_instances);
		
	world->type_registry[desc->type] = *desc;
}

static void E_WorldTickPreAnim(E_World *world, E_EventQueue *events, f32 dt, const OS_InputState *input)
{
	E_TickContext ctx = {0};
	ctx.world = world;
	ctx.events = events;
	ctx.input = input;
	ctx.dt = dt;
	
	for (u32 t = 0; t < E_Type_COUNT; t++)
	{
		const E_TypeDesc *desc = &world->type_registry[t];

		if (!desc->OnPreAnimTick)
			continue;

		E_TypePool *pool = &world->type_pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			E_Flags flags = E_FlagsOf(entity);
			u16 layer_id = E_LayerIDOf(entity);
			
			if (!(flags & E_Flag_Active))
				continue;

			if (!world->layers[layer_id].active)
				continue;

			desc->OnPreAnimTick(entity, &ctx);
		}
	}
}

static void E_WorldTickPostAnim(E_World *world, E_EventQueue *events, f32 dt, const OS_InputState *input)
{
	E_TickContext ctx = {0};
	ctx.world = world;
	ctx.events = events;
	ctx.input = input;
	ctx.dt = dt;
	
	for (u32 t = 0; t < E_Type_COUNT; t++)
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

			E_Flags flags = E_FlagsOf(entity);
			u16 layer_id = E_LayerIDOf(entity);
			
			if (!(flags & E_Flag_Active))
				continue;

			if (!world->layers[layer_id].active)
				continue;

			desc->OnPostAnimTick(entity, &ctx);
		}
	}
}

static void E_WorldTickPostPhysics(E_World *world, E_EventQueue *events, f32 dt, const OS_InputState *input)
{
	E_TickContext ctx = {0};
	ctx.world = world;
	ctx.events = events;
	ctx.input = input;
	ctx.dt = dt;
	
	for (u32 t = 0; t < E_Type_COUNT; t++)
	{
		const E_TypeDesc *desc = &world->type_registry[t];

		if (!desc->OnPostPhysicsTick)
			continue;

		E_TypePool *pool = &world->type_pools[t];
		
		u8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			E_Flags flags = E_FlagsOf(entity);
			u16 layer_id = E_LayerIDOf(entity);
			
			if (!(flags & E_Flag_Active))
				continue;

			if (!world->layers[layer_id].active)
				continue;

			desc->OnPostPhysicsTick(entity, &ctx);
		}
	}
}

static void E_WorldFlush(E_World *world)
{
	for (u32 t = 0; t < E_Type_COUNT; t++)
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

			E_Flags flags = E_FlagsOf(entity);

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

static void *E_WorldSpawn(E_World *world, E_Type type)
{
	const E_TypeDesc *desc = &world->type_registry[type];
	E_TypePool *pool = &world->type_pools[type];

	u32 slot = E_PoolAllocSlot(pool);

	void *entity = (void *)(pool->data + (slot * desc->stride));

	MemZero(entity, desc->stride);

	pool->alive[slot] = true;

	E_Header *header = E_HeaderOf(entity);
	header->uid.value = world->next_uid;
	header->type = type;
	header->flags = E_Flag_Active | E_Flag_Visible;
	header->layer_id = 0;

	E_TransformSetPosition (&header->transform, v3x(0.f));
	E_TransformSetRotation (&header->transform, V4QuatIdentity());
	E_TransformSetScale    (&header->transform, v3x(1.f));
	E_TransformSetOrigin   (&header->transform, v3x(0.f));
	
	E_TransformRecompute(&header->transform);
	
	world->next_uid++;

	return entity;
}

static void E_WorldKill(E_World *world, E_UID uid)
{
	void *entity = E_WorldGet(world, uid);

	if (entity)
	{
		E_Header *header = E_HeaderOf(entity);
		header->flags |= E_Flag_PendingKill;
	}
}

static void *E_WorldGet(E_World *world, E_UID uid)
{
	for (u32 t = 0; t < E_Type_COUNT; t++)
	{
		const E_TypeDesc *desc = &world->type_registry[t];

		E_TypePool *pool = &world->type_pools[t];

		b8 *base = pool->data;

		for (u32 j = 0; j < pool->count; j++)
		{
			if (!pool->alive[j])
				continue;

			void *entity = (void *)(base + (j * desc->stride));

			if (E_UIDMatch(E_UIDOf(entity), uid))
				return entity;
		}
	}

	DebugLogW(world->log_channel, "Couldn't find entity with UID %u", uid.value);

	return NULL;
}

static E_GetAllReceipt E_WorldGetAll(E_World *world, E_Type type)
{
	E_TypePool *pool = &world->type_pools[type];

	E_GetAllReceipt receipt = {0};
	receipt.data = pool->data;
	receipt.count = pool->count;
	receipt.alive = pool->alive;
	receipt.stride = world->type_registry[type].stride;

	return receipt;
}

static E_Marker *E_WorldAddMarker(E_World *world, String8 name, v3 position, v4 rotation, u16 layer_id)
{
	AssertTrue(world->marker_count < ArraySize(world->markers));
	
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
