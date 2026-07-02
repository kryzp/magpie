#ifndef PHYSICS_SHAPE_H
#define PHYSICS_SHAPE_H

typedef enum P_CollisionShapeType
{
	P_CollisionShapeType_None,
	P_CollisionShapeType_Box,
	P_CollisionShapeType_COUNT
}
P_CollisionShapeType;

typedef struct P_CollisionShape P_CollisionShape;
struct P_CollisionShape
{
	P_CollisionShapeType type;

	union
	{
		struct
		{
			v3 local_bounds_position;
			v3 local_bounds_size;
		}
		box;
	};
};

#endif // PHYSICS_SHAPE_H
