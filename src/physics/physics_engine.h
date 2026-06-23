#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

typedef struct P_Engine P_Engine;
struct P_Engine
{
	LOG_Channel log_channel;
};

static void P_EngineInit    (P_Engine *engine, LOG_Channel log_channel);
static void P_EngineDestroy (P_Engine *engine);
static void P_EngineTick    (P_Engine *engine, f32 dt);

#endif // PHYSICS_ENGINE_H
