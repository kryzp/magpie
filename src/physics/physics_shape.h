#ifndef PHYSICS_SHAPE_H
#define PHYSICS_SHAPE_H

typedef enum P_CollisionShapeType
{
	P_CollisionShapeType_COUNT
}
P_CollisionShapeType;

typedef struct P_CollisionShape P_CollisionShape;
struct P_CollisionShape
{
	P_CollisionShapeType type;
};

#endif // PHYSICS_SHAPE_H
