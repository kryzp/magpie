
internal void
E_TransformRecompute(E_Transform *transform)
{
	transform->matrix = M4Transform(transform->position,
									transform->rotation,
									transform->scale,
									transform->origin);
	
	transform->dirty = false;
}

internal m4
E_TransformGetMatrix(E_Transform *transform)
{
	if (transform->dirty)
		E_TransformRecompute(transform);

	return transform->matrix;
}

internal void
E_TransformSetPosition(E_Transform *transform, v3 position)
{
	transform->position = position;
	transform->dirty = true;
}

internal void
E_TransformSetRotation(E_Transform *transform, v4 rotation)
{
	transform->rotation = rotation;
	transform->dirty = true;
}

internal void
E_TransformSetScale(E_Transform *transform, v3 scale)
{
	transform->scale = scale;
	transform->dirty = true;
}

internal void
E_TransformSetOrigin(E_Transform *transform, v3 origin)
{
	transform->origin = origin;
	transform->dirty = true;
}
