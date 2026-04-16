
internal void
ENT_TransformRecompute(ENT_Transform *transform)
{
	transform->matrix = M4Transform(transform->position,
									transform->rotation,
									transform->scale,
									transform->origin);
	
	transform->dirty = false;
}

internal m4
ENT_TransformGetMatrix(ENT_Transform *transform)
{
	if (transform->dirty)
		ENT_TransformRecompute(transform);

	return transform->matrix;
}

internal void
ENT_TransformSetPosition(ENT_Transform *transform, v3 position)
{
	transform->position = position;
	transform->dirty = true;
}

internal void
ENT_TransformSetRotation(ENT_Transform *transform, v4 rotation)
{
	transform->rotation = rotation;
	transform->dirty = true;
}

internal void
ENT_TransformSetScale(ENT_Transform *transform, v3 scale)
{
	transform->scale = scale;
	transform->dirty = true;
}

internal void
ENT_TransformSetOrigin(ENT_Transform *transform, v3 origin)
{
	transform->origin = origin;
	transform->dirty = true;
}
