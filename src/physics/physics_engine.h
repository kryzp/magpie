#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#define P_GRAVITY_STRENGTH 10.f

typedef struct P_Instance P_Instance;
struct P_Instance
{
	P_Instance *next;
	P_Instance *prev;

	u64 key;

	P_RigidBody rigidbody;
};

typedef struct P_Engine P_Engine;
struct P_Engine
{
	Arena *arena;
	LOG_Channel log_channel;

	P_Instance instance_sentinel;
	P_Instance free_instance_sentinel;
	u64 current_key;
};

static void P_EngineInitAndSelect(P_Engine *engine, Arena *arena, LOG_Channel log_channel);
static void P_EngineDestroy(void);
static void P_EngineSelectContext(P_Engine *engine);
static void P_EngineTick(f32 dt);

static P_Handle P_LeaseInstance(void);
static void P_ReturnInstance(P_Handle handle);

static P_RigidBody *P_GetRigidbodyFromHandle(P_Handle handle);

static J_ENTRY_POINT_DEF(P_CastRayJob);

static P_Raycast P_CastRay(v3 start_position, v3 direction, OS_Handle counter);
static P_Raycast P_CastRayEx(v3 start_position, v3 direction, f32 dt, u32 max_steps, OS_Handle counter);

#endif // PHYSICS_ENGINE_H
