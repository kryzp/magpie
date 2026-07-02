#ifndef PHYSICS_HIT_H
#define PHYSICS_HIT_H

typedef struct P_Hit P_Hit;
struct P_Hit
{
    v3 pushout;
    P_Handle other;
};

typedef struct P_Raycast P_Raycast;
struct P_Raycast
{
    v3 start_position;
    v3 direction;
    f32 time;
    P_Hit hit;
};

static v3 P_RaycastCalcFinalPosition(const P_Raycast *ray);
static v3 P_RaycastCalcPositionAt(const P_Raycast *ray, f32 t);

#endif // PHYSICS_HIT_H
