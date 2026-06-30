#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

typedef struct P_Handle P_Handle;
struct P_Handle
{
	u32 value;
};

static inline P_Handle P_HandleNull(void)
{
	P_Handle null_handle = {0};
	return null_handle;
}

static inline b32 P_HandleIsNull(P_Handle handle)
{
	return handle.value == 0;
}

static inline b32 P_HandleMatch(P_Handle a, P_Handle b)
{
	return a.value == b.value;
}

typedef struct P_Engine P_Engine;
struct P_Engine
{
	LOG_Channel log_channel;
};

static void P_EngineInit(P_Engine *engine, LOG_Channel log_channel);
static void P_EngineDestroy(P_Engine *engine);
static void P_EngineTick(P_Engine *engine, f32 dt);

#endif // PHYSICS_ENGINE_H
