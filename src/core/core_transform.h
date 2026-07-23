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

internal Transform TransformIdentity(void);

internal void TransformRecompute(Transform *transform);
internal m4 TransformMatrix(Transform *transform);

internal void TransformSetPosition(Transform *transform, v3 position);
internal void TransformMoveBy(Transform *transform, v3 by);

internal void TransformSetRotation(Transform *transform, v4 rotation);
internal void TransformSetScale(Transform *transform, v3 scale);
internal void TransformSetOrigin(Transform *transform, v3 origin);

#endif // CORE_TRANSFORM_H
