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

internal void P_EngineInitAndSelect(P_Engine *engine, Arena *arena, LOG_Channel log_channel);
internal void P_EngineDestroy(void);
internal void P_EngineSelectContext(P_Engine *engine);
internal void P_EngineTick(f32 dt);

internal P_Handle P_LeaseInstance(void);
internal void P_ReturnInstance(P_Handle handle);

internal P_RigidBody *P_GetRigidbodyFromHandle(P_Handle handle);

internal J_ENTRY_POINT_DEF(P_CastRayJob);

internal P_Raycast P_CastRay(v3 start_position, v3 direction, OS_Handle counter);
internal P_Raycast P_CastRayEx(v3 start_position, v3 direction, f32 dt, u32 max_steps, OS_Handle counter);

#endif // PHYSICS_ENGINE_H
