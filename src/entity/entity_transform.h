#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

typedef struct ENT_Transform ENT_Transform;
struct ENT_Transform
{
	v3 position;
	v4 rotation;
	v3 scale;
	v3 origin;
	m4 matrix;
	b32 dirty;
};

internal void ENT_TransformRecompute(ENT_Transform *transform);
internal m4   ENT_TransformGetMatrix(ENT_Transform *transform);

// I hate getters / setters but we need these to automatically
// set the dirty flag.
internal void ENT_TransformSetPosition (ENT_Transform *transform, v3 position);
internal void ENT_TransformSetRotation (ENT_Transform *transform, v4 rotation);
internal void ENT_TransformSetScale    (ENT_Transform *transform, v3 scale);
internal void ENT_TransformSetOrigin   (ENT_Transform *transform, v3 origin);

#endif // ENTITY_TRANSFORM_H
