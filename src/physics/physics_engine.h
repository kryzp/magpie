#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

typedef struct PHYS_Engine PHYS_Engine;
struct PHYS_Engine
{
	LOG_Channel log_channel;
};

internal void PHYS_EngineInit    (PHYS_Engine *engine, LOG_Channel log_channel);
internal void PHYS_EngineDestroy (PHYS_Engine *engine);
internal void PHYS_EngineTick    (PHYS_Engine *engine, f32 dt);

#endif // PHYSICS_ENGINE_H
