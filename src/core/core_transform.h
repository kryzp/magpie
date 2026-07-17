#ifndef CORE_TRANSFORM_H
#define CORE_TRANSFORM_H

typedef struct Transform Transform;
struct Transform
{
	v3 position;
	v4 rotation;
	v3 scale;
	v3 origin;

	m4 matrix;
	b32 dirty;
};

static Transform TransformIdentity(void);

static void TransformRecompute(Transform *transform);
static m4 TransformMatrix(Transform *transform);

static void TransformSetPosition(Transform *transform, v3 position);
static void TransformMoveBy(Transform *transform, v3 by);

static void TransformSetRotation(Transform *transform, v4 rotation);
static void TransformSetScale(Transform *transform, v3 scale);
static void TransformSetOrigin(Transform *transform, v3 origin);

#endif // CORE_TRANSFORM_H
