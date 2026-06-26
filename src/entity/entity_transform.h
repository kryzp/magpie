#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

typedef struct E_Transform E_Transform;
struct E_Transform
{
	v3 position;
	v4 rotation;
	v3 scale;
	v3 origin;

	m4 matrix;
	b32 dirty;
};

static E_Transform E_TransformIdentity(void);

static void E_TransformRecompute(E_Transform *transform);
static m4 E_TransformMatrix(E_Transform *transform);

static void E_TransformSetPosition(E_Transform *transform, v3 position);
static void E_TransformMoveBy(E_Transform *transform, v3 by);

static void E_TransformSetRotation(E_Transform *transform, v4 rotation);
static void E_TransformSetScale(E_Transform *transform, v3 scale);
static void E_TransformSetOrigin(E_Transform *transform, v3 origin);

#endif // ENTITY_TRANSFORM_H
