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

	v3 position;
	v4 orientation;

	v3 velocity;
	v3 acceleration;
	v3 last_position;

	b32 fixed_position;
	b32 solid;

	f32 friction;
	f32 air_friction;

	f32 max_speed;
	f32 max_z_speed;

	f32 gravity_factor;
};

#endif // PHYSICS_RIGIDBODY_H
