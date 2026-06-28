#ifndef PHYSICS_RIGIDBODY_H
#define PHYSICS_RIGIDBODY_H

typedef enum P_RigidBodyType
{
	P_RigidBodyType_Static,
	P_RigidBodyType_Dynamic,
	P_RigidBodyType_Kinematic,
	P_RigidBodyType_COUNT
}
P_RigidBodyType;

typedef struct P_RigidBody P_RigidBody;
struct P_RigidBody
{
	P_RigidBodyType type;
	P_CollisionShape shape;
};

#endif // PHYSICS_RIGIDBODY_H
