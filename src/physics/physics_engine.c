
static void P_EngineInit(P_Engine *engine, Arena *arena, LOG_Channel log_channel)
{
	engine->arena = arena;
	engine->log_channel = log_channel;

	engine->current_key = 0;
	
	engine->instance_sentinel.next = &engine->instance_sentinel;
	engine->instance_sentinel.prev = &engine->instance_sentinel;

	engine->free_instance_sentinel.next = &engine->free_instance_sentinel;
	engine->free_instance_sentinel.prev = &engine->free_instance_sentinel;
}

static void P_EngineDestroy(P_Engine *engine)
{
}

static void P_EngineTick(P_Engine *engine, f32 dt)
{
	for (P_Instance *inst = engine->instance_sentinel.next; 
		 inst != &engine->instance_sentinel; 
		 inst = inst->next)
	{
		// took this from an old game project in FNA which
		// I believe itself was taken from the CrossCode physics
		// engine blog posts.
		
		// was a 2d physics engine but semi-3d (levels) so I'm
		// just repurposing it here until I bother to look further
		// into physics engines.

		// I just need it to be good enough to *feel* good, not necessarily
		// be physically accurate :)

		P_RigidBody *rb = &inst->rigidbody;

		b32 is_above_ground = false;

		f32 friction = is_above_ground ? rb->air_friction : rb->friction;
		f32 frictional_factor = ClampValue(friction, 0.f, 1.f / dt);

		v3 friction_acc = V3MulF32(rb->acceleration, rb->max_speed * frictional_factor * dt);
		v3 friction_vel = V3MulF32(rb->velocity, 1.2f * frictional_factor * dt);

		f32 speed = V3Length(rb->velocity);

		if (speed <= rb->max_speed)
		{
			v3 accel_dir = V3Normalize(rb->acceleration);
			f32 dot = V3Dot(accel_dir, friction_vel);

			if (dot > 0.f)
				friction_vel = V3Sub(friction_vel, V3MulF32(accel_dir, dot));
		}

		rb->velocity = V3Add(rb->velocity, V3MulF32(friction_acc, dt));

		if (rb->position.z > 0.f)
			rb->velocity.z -= P_GRAVITY_STRENGTH * rb->gravity_factor * dt;
		else
			rb->velocity.z = MaxValue(rb->velocity.z, 0.f);

		rb->velocity = V3Sub(rb->velocity, friction_vel);

		rb->last_position = rb->position;

		if (rb->shape.type != P_CollisionShapeType_None && !WithinEpsilon(speed))
		{
			f32 dx = speed * dt;
			v3 dir = V3Normalize(rb->velocity);

			for (u32 i = 0; i < 16; i++)
			{
				f32 step_dist = dx / (f32)16;
				v3 step = V3MulF32(dir, step_dist);

				rb->position = V3Add(rb->position, step);

				//P_CheckForCollision(rb);
			}

			//P_CheckForCollision(rb);
		}
		else
		{
			rb->position = V3Add(rb->position, V3MulF32(rb->velocity, dt));
		}
	}
}

static P_Handle P_LeaseInstance(P_Engine *engine)
{
	P_Instance *inst = NULL;

	if (engine->free_instance_sentinel.next != &engine->free_instance_sentinel)
	{
		inst = engine->free_instance_sentinel.next;
		inst->prev->next = inst->next;
		inst->next->prev = inst->prev;

		MemZeroStruct(inst);
	}
	else
	{
		inst = ArenaPushArray(engine->arena, P_Instance, 1);
	}

	inst->key = engine->current_key;
	engine->current_key++;

	inst->next = engine->instance_sentinel.next;
	inst->prev = &engine->instance_sentinel;

	inst->next->prev = inst;
	inst->prev->next = inst;

	P_Handle handle = {0};
	handle.key = inst->key;

	return handle;
}

static void P_ReturnInstance(P_Engine *engine, P_Handle handle)
{
	for (P_Instance *inst = engine->instance_sentinel.next; 
		 inst != &engine->instance_sentinel; 
		 inst = inst->next)
	{
		if (inst->key != handle.key)
			continue;
	
		inst->prev->next = inst->next;
		inst->next->prev = inst->prev;

		inst->next = engine->free_instance_sentinel.next;
		inst->prev = &engine->free_instance_sentinel;

		inst->next->prev = inst;
		inst->prev->next = inst;
	}
}

static P_RigidBody *P_GetRigidbodyFromHandle(P_Engine *engine, P_Handle handle)
{
	for (P_Instance *inst = engine->instance_sentinel.next; 
		 inst != &engine->instance_sentinel; 
		 inst = inst->next)
	{
		if (inst->key == handle.key)
			return &inst->rigidbody;
	}

	return NULL;
}

static J_ENTRY_POINT_DEF(P_CastRayJob)
{
}

static P_Raycast P_CastRay(P_Engine *engine, v3 start_position, v3 direction, OS_Handle counter)
{
	return P_CastRayEx(engine, start_position, direction, 1.f, 512, counter);
}

static P_Raycast P_CastRayEx(P_Engine *engine, v3 start_position, v3 direction, f32 dt, u32 max_steps, OS_Handle counter)
{
	// TODO: make job-based. return P_RaycastHandle?
	/*
	P_Raycast ray = {0};

	b32 is_hit = false;

	for (u32 i = 0; i < max_steps; i++)
	{
		v3 pos = P_RaycastCalcPositionAt(&ray, ray.time);

		if (is_hit)
			break;

		ray.time += dt;

		// TODO
	}

	return ray;
	*/
}
