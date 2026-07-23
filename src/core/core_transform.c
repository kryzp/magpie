
internal Transform TransformIdentity(void)
{
	Transform transform = {0};
	transform.position = v3x(0.f);
	transform.rotation = V4QuatIdentity();
	transform.scale = v3x(1.f);
	transform.origin = v3x(0.f);

	TransformRecompute(&transform);
	
	return transform;
}

internal void TransformRecompute(Transform *transform)
{
	transform->matrix = M4Transform(transform->position,
									transform->rotation,
									transform->scale,
									transform->origin);

	transform->dirty = false;
}

internal m4 TransformMatrix(Transform *transform)
{
	if (transform->dirty)
		TransformRecompute(transform);

	return transform->matrix;
}

internal void TransformSetPosition(Transform *transform, v3 position)
{
	transform->position = position;
	transform->dirty = true;
}

internal void TransformMoveBy(Transform *transform, v3 by)
{
	transform->position = V3Add(transform->position, by);
	transform->dirty = true;
}

internal void TransformSetRotation(Transform *transform, v4 rotation)
{
	transform->rotation = rotation;
	transform->dirty = true;
}

internal void TransformSetScale(Transform *transform, v3 scale)
{
	transform->scale = scale;
	transform->dirty = true;
}

internal void TransformSetOrigin(Transform *transform, v3 origin)
{
	transform->origin = origin;
	transform->dirty = true;
}
